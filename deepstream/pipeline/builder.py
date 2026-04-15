import os
import logging

import yaml

from pyservicemaker import Pipeline, Probe, Receiver

from pipeline.analytics_probe import AnalyticsMetadataProbe
from pipeline.osd_toggle import OsdToggle
from pipeline.screenshot import ScreenshotRetriever
from pipeline.yolo_postprocessor import YoloV10Postprocessor
from utils.storage import StorageManager

logger = logging.getLogger(__name__)


class PipelineComponents:
    """Holds references to pipeline elements and helpers needed by main.py."""

    def __init__(self):
        self.pipeline = None
        self.screenshot_retriever = None
        self.tiler_element = None
        self.pgie_element = None
        self.tracker_element = None
        self.source_element = None
        self.osd_toggle = None


class PipelineBuilder:
    """Construct the DeepStream pipeline from environment variables and config files.

    Pipeline topology:
        nvmultiurisrcbin -> nvinfer(PGIE) -> nvtracker -> [nvdsanalytics] -> tee
            ├─ queue_meta   -> nvmsgconv -> nvmsgbroker (Kafka)
            ├─ queue_snap   -> valve -> nvvideoconvert -> jpegenc -> appsink (screenshot)
            └─ queue_preview -> tiler -> nvosd (OSD toggle via display-bbox/text) -> nvvideoconvert
                              -> nvv4l2h264enc -> rtph264pay -> udpsink (→ MediaMTX RTP source)
    """

    def __init__(self, storage: StorageManager):
        self._storage = storage
        self._kafka_broker = os.environ.get("KAFKA_BROKER", "kafka:9092")
        self._kafka_topic = os.environ.get("KAFKA_TOPIC", "deepstream-detections")
        self._kafka_event_topic = os.environ.get("KAFKA_EVENT_TOPIC", "deepstream-events")
        self._pipeline_width = int(os.environ.get("DS_PIPELINE_WIDTH", "1920"))
        self._pipeline_height = int(os.environ.get("DS_PIPELINE_HEIGHT", "1080"))
        self._rest_port = int(os.environ.get("DS_REST_PORT", "9000"))
        self._max_batch = int(os.environ.get("DS_MAX_BATCH_SIZE", "16"))
        self._pgie_config = os.environ.get("DS_PGIE_CONFIG", "/app/config/pgie_yolov10_config.yml")
        self._tracker_ll_config = os.environ.get(
            "DS_TRACKER_CONFIG",
            "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml",
        )
        self._analytics_config = os.environ.get("DS_ANALYTICS_CONFIG", "")
        self._preview_bitrate = int(os.environ.get("DS_PREVIEW_BITRATE", "4000000"))
        self._tiler_rows = int(os.environ.get("DS_PREVIEW_TILER_ROWS", "4"))
        self._tiler_cols = int(os.environ.get("DS_PREVIEW_TILER_COLS", "4"))
        self._light_pipeline = os.environ.get("DS_LIGHT_PIPELINE", "1") == "1"
        self._preview_rtp_port = int(os.environ.get("DS_PREVIEW_RTP_PORT", "5400"))
        self._yolo_person_only = os.environ.get("DS_YOLO_PERSON_ONLY", "1") == "1"
        self._yolo_threshold = float(os.environ.get("DS_YOLO_THRESHOLD", "0.3"))
        self._labels_path = os.environ.get("DS_LABELS_PATH", "/app/models/coco_labels.txt")

        self._recording_dir = str(storage.buffer_dir)
        self._sr_cache = int(os.environ.get("DS_SR_CACHE_SEC", "30"))
        self._sr_default_duration = int(os.environ.get("DS_SR_DEFAULT_DURATION", "20"))

    # ------------------------------------------------------------------
    # public
    # ------------------------------------------------------------------

    def build(self) -> PipelineComponents:
        comp = PipelineComponents()
        pipeline = Pipeline("deepstream-pipeline")
        comp.pipeline = pipeline

        self._add_source(pipeline, comp)
        self._add_inference(pipeline)
        self._add_tracker(pipeline)
        analytics_enabled = self._add_analytics(pipeline)
        self._add_tee(pipeline, analytics_enabled)
        self._add_kafka_branch(pipeline, analytics_enabled)
        self._add_snapshot_branch(pipeline, comp)
        self._add_preview_branch(pipeline, comp)

        comp.pgie_element = pipeline["pgie"]
        comp.tracker_element = pipeline["tracker"]
        return comp

    # ------------------------------------------------------------------
    # source
    # ------------------------------------------------------------------

    def _add_source(self, pipeline: Pipeline, comp: PipelineComponents):
        source_props = {
            "ip-address": "0.0.0.0",
            "port": self._rest_port,
            "max-batch-size": self._max_batch,
            "batched-push-timeout": 33333,
            "width": self._pipeline_width,
            "height": self._pipeline_height,
            "live-source": 1,
            "drop-pipeline-eos": 1,
            "async-handling": 1,
            "select-rtp-protocol": 0,
            "latency": 100,
            "file-loop": 1,
        }

        source_props.update({
            "smart-record": 2,
            "smart-rec-dir-path": self._recording_dir,
            "smart-rec-file-prefix": "sr",
            "smart-rec-cache": self._sr_cache,
            "smart-rec-default-duration": self._sr_default_duration,
            "smart-rec-container": 0,
        })

        pipeline.add("nvmultiurisrcbin", "src", source_props)
        comp.source_element = pipeline["src"]

    # ------------------------------------------------------------------
    # inference
    # ------------------------------------------------------------------

    def _add_inference(self, pipeline: Pipeline):
        if self._light_pipeline:
            pipeline.add("identity", "pgie")
            return
        pipeline.add("nvinfer", "pgie", {
            "config-file-path": self._pgie_config,
            "unique-id": 1,
        })
        if self._needs_yolo_postprocessor():
            postprocessor = YoloV10Postprocessor(
                confidence_threshold=self._yolo_threshold,
                person_only=self._yolo_person_only,
                labels_path=self._labels_path,
            )
            pipeline.attach("pgie", Probe("yolo-postprocess", postprocessor))

    def _needs_yolo_postprocessor(self) -> bool:
        """Check if the PGIE config uses output-tensor-meta (custom YOLO parsing)."""
        try:
            with open(self._pgie_config) as f:
                cfg = yaml.safe_load(f)
            return bool(cfg.get("property", {}).get("output-tensor-meta"))
        except (OSError, yaml.YAMLError):
            return False

    # ------------------------------------------------------------------
    # tracker
    # ------------------------------------------------------------------

    def _add_tracker(self, pipeline: Pipeline):
        if self._light_pipeline:
            pipeline.add("identity", "tracker")
            return
        pipeline.add("nvtracker", "tracker", {
            "ll-lib-file": "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
            "ll-config-file": self._tracker_ll_config,
        })

    # ------------------------------------------------------------------
    # analytics (optional)
    # ------------------------------------------------------------------

    def _add_analytics(self, pipeline: Pipeline) -> bool:
        if not self._analytics_config:
            return False
        pipeline.add("nvdsanalytics", "analytics", {
            "config-file": self._analytics_config,
        })
        return True

    # ------------------------------------------------------------------
    # tee + main link
    # ------------------------------------------------------------------

    def _add_tee(self, pipeline: Pipeline, analytics_enabled: bool):
        pipeline.add("tee", "tee")

        elements = ["src", "pgie", "tracker"]
        if analytics_enabled:
            elements.append("analytics")
        elements.append("tee")
        pipeline.link(*elements)

    # ------------------------------------------------------------------
    # branch 1 — Kafka metadata
    # ------------------------------------------------------------------

    def _add_kafka_branch(self, pipeline: Pipeline, analytics_enabled: bool):
        if self._light_pipeline:
            pipeline.add("queue", "queue_meta")
            pipeline.add("fakesink", "msgbroker", {"sync": 0, "async": 0})
            pipeline.link(("tee", "queue_meta"), ("src_%u", ""))
            pipeline.link("queue_meta", "msgbroker")
            return

        pipeline.add("queue", "queue_meta")

        kafka_conn = self._kafka_broker.replace(":", ";")

        pipeline.add("nvmsgconv", "msgconv", {
            "config": "/app/config/msgconv_config.txt",
            "payload-type": 1,
            "msg2p-newapi": True,
        })

        pipeline.add("nvmsgbroker", "msgbroker", {
            "proto-lib": "/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so",
            "conn-str": kafka_conn,
            "topic": self._kafka_topic,
            "sync": 0,
            "async": 0,
            "config": "/app/config/kafka_broker_config.txt",
        })

        pipeline.link(("tee", "queue_meta"), ("src_%u", ""))
        pipeline.link("queue_meta", "msgconv", "msgbroker")

        if analytics_enabled:
            analytics_probe = AnalyticsMetadataProbe()
            pipeline.attach("analytics", Probe("analytics-probe", analytics_probe))

    # ------------------------------------------------------------------
    # branch 2 — snapshot
    # ------------------------------------------------------------------

    def _add_snapshot_branch(self, pipeline: Pipeline, comp: PipelineComponents):
        pipeline.add("queue", "queue_snap")
        pipeline.add("valve", "snap_valve", {"drop": True})
        pipeline.add("nvvideoconvert", "snap_convert")
        pipeline.add("capsfilter", "snap_caps", {"caps": "video/x-raw(memory:NVMM),format=RGB"})
        pipeline.add("appsink", "snap_sink", {"emit-signals": True, "sync": 0, "async": 0})

        pipeline.link(("tee", "queue_snap"), ("src_%u", ""))
        pipeline.link("queue_snap", "snap_valve", "snap_convert", "snap_caps", "snap_sink")

        screenshot_retriever = ScreenshotRetriever(
            storage=self._storage,
            valve_element=pipeline["snap_valve"],
            kafka_broker=self._kafka_broker,
            kafka_topic=self._kafka_event_topic,
        )
        pipeline.attach("snap_sink", Receiver("snap-receiver", screenshot_retriever), tips="new-sample")
        comp.screenshot_retriever = screenshot_retriever

    # ------------------------------------------------------------------
    # branch 3 — preview (tiler + OSD + RTSP)
    # ------------------------------------------------------------------

    def _add_preview_branch(self, pipeline: Pipeline, comp: PipelineComponents):
        pipeline.add("queue", "queue_preview")

        pipeline.add("nvmultistreamtiler", "tiler", {
            "rows": self._tiler_rows,
            "columns": self._tiler_cols,
            "width": self._pipeline_width,
            "height": self._pipeline_height,
            "show-source": -1,
        })

        pipeline.add("nvdsosd", "osd")
        pipeline.add("nvvideoconvert", "preview_convert")

        pipeline.add("nvv4l2h264enc", "encoder", {
            "bitrate": self._preview_bitrate,
            "iframeinterval": 30,
        })

        pipeline.add("rtph264pay", "rtppay", {"pt": 96})

        pipeline.add("udpsink", "preview_udpsink", {
            "host": "127.0.0.1",
            "port": self._preview_rtp_port,
            "sync": 0,
            "async": 0,
        })

        pipeline.link(("tee", "queue_preview"), ("src_%u", ""))
        pipeline.link(
            "queue_preview", "tiler", "osd", "preview_convert",
            "encoder", "rtppay", "preview_udpsink",
        )

        comp.tiler_element = pipeline["tiler"]
        comp.osd_toggle = OsdToggle(pipeline["osd"])
