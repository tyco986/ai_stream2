import os
import logging

from pyservicemaker import (
    Pipeline, Probe, Receiver, SmartRecordConfig,
)

from analytics_probe import AnalyticsMetadataProbe
from empty_frame_filter import EmptyFrameFilter
from screenshot_handler import ScreenshotRetriever

logger = logging.getLogger(__name__)


class PipelineComponents:
    """Holds references to pipeline elements and helpers needed by main.py."""

    def __init__(self):
        self.pipeline = None
        self.sr_controller = None
        self.screenshot_retriever = None
        self.tiler_element = None
        self.pgie_element = None
        self.tracker_element = None


class PipelineBuilder:
    """Construct the DeepStream pipeline from environment variables and config files.

    Pipeline topology (see docs/plan-deepstream.md Section 2):
        nvmultiurisrcbin -> nvinfer(PGIE) -> nvtracker -> [nvdsanalytics] -> tee
            ├─ queue_meta   -> EmptyFrameFilter(probe) -> nvmsgconv -> nvmsgbroker (Kafka)
            ├─ queue_snap   -> valve -> nvvideoconvert -> jpegenc -> appsink (screenshot)
            └─ queue_preview -> tiler -> nvosd -> nvvideoconvert -> nvv4l2h264enc -> rtppay -> udpsink (RTSP)
    """

    def __init__(self):
        self._kafka_broker = os.environ.get("KAFKA_BROKER", "kafka:9092")
        self._kafka_topic = os.environ.get("KAFKA_TOPIC", "deepstream-detections")
        self._kafka_event_topic = os.environ.get("KAFKA_EVENT_TOPIC", "deepstream-events")
        self._rest_port = int(os.environ.get("DS_REST_PORT", "9000"))
        self._max_batch = int(os.environ.get("DS_MAX_BATCH_SIZE", "16"))
        self._pgie_config = os.environ.get("DS_PGIE_CONFIG", "/app/config/pgie_config.yml")
        self._tracker_ll_config = os.environ.get(
            "DS_TRACKER_CONFIG",
            "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml",
        )
        self._analytics_config = os.environ.get("DS_ANALYTICS_CONFIG", "")
        self._rolling_dir = os.environ.get("DS_ROLLING_DIR", "/app/recordings/rolling")
        self._locked_dir = os.environ.get("DS_LOCKED_DIR", "/app/recordings/locked")
        self._screenshot_dir = os.environ.get("DS_SCREENSHOT_DIR", "/app/screenshots")
        self._recording_cache_sec = int(os.environ.get("DS_RECORDING_CACHE_SEC", "30"))
        self._rtsp_port = os.environ.get("DS_RTSP_PORT", "8554")
        self._preview_bitrate = int(os.environ.get("DS_PREVIEW_BITRATE", "4000000"))
        self._tiler_rows = int(os.environ.get("DS_PREVIEW_TILER_ROWS", "4"))
        self._tiler_cols = int(os.environ.get("DS_PREVIEW_TILER_COLS", "4"))
        self._empty_frame_filter = int(os.environ.get("DS_EMPTY_FRAME_FILTER", "1"))
        self._udpsink_port = 5400

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
        kafka_conn = self._kafka_broker.replace(":", ";")

        sr_config = SmartRecordConfig(
            smart_rec_cache=self._recording_cache_sec,
            smart_rec_container=0,
            smart_rec_dir_path=self._rolling_dir,
            smart_rec_mode=1,
            proto_lib="/opt/nvidia/deepstream/deepstream/lib/libnvds_kafka_proto.so",
            conn_str=kafka_conn,
            msgconv_config_file="/app/config/msgconv_config.txt",
            proto_config_file="/app/config/kafka_broker_config.txt",
            topic_list=self._kafka_event_topic,
        )

        source_props = {
            "ip-address": "0.0.0.0",
            "port": self._rest_port,
            "max-batch-size": self._max_batch,
            "batched-push-timeout": 33333,
            "width": 1920,
            "height": 1080,
            "live-source": 1,
            "drop-pipeline-eos": 1,
            "async-handling": 1,
            "select-rtp-protocol": 0,
            "latency": 100,
            "smart-rec-cache": sr_config.smart_rec_cache,
            "smart-rec-container": sr_config.smart_rec_container,
            "smart-rec-dir-path": sr_config.smart_rec_dir_path,
            "smart-rec-mode": sr_config.smart_rec_mode,
            # SmartRecord native Kafka notification: sr-done events are
            # published to deepstream-events topic automatically via
            # libnvds_kafka_proto.so — no Python-side Kafka Producer needed.
            "smart-rec-proto-lib": sr_config.proto_lib,
            "smart-rec-conn-str": sr_config.conn_str,
            "smart-rec-msgconv-config-file": sr_config.msgconv_config_file,
            "smart-rec-proto-config-file": sr_config.proto_config_file,
            "smart-rec-topic-list": sr_config.topic_list,
        }

        pipeline.add("nvmultiurisrcbin", "src", source_props)
        comp.sr_controller = pipeline["src"]

    # ------------------------------------------------------------------
    # inference
    # ------------------------------------------------------------------

    def _add_inference(self, pipeline: Pipeline):
        pipeline.add("nvinfer", "pgie", {
            "config-file-path": self._pgie_config,
            "batch-size": self._max_batch,
        })

    # ------------------------------------------------------------------
    # tracker
    # ------------------------------------------------------------------

    def _add_tracker(self, pipeline: Pipeline):
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

        if self._empty_frame_filter:
            empty_filter = EmptyFrameFilter()
            pipeline.attach("queue_meta", Probe("empty-frame-filter", empty_filter))

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
        pipeline.add("jpegenc", "snap_jpegenc", {"quality": 95})
        pipeline.add("appsink", "snap_sink", {"emit-signals": True, "sync": 0, "async": 0})

        pipeline.link(("tee", "queue_snap"), ("src_%u", ""))
        pipeline.link("queue_snap", "snap_valve", "snap_convert", "snap_jpegenc", "snap_sink")

        screenshot_retriever = ScreenshotRetriever(
            output_dir=self._screenshot_dir,
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
            "width": 1920,
            "height": 1080,
            "show-source": -1,
        })

        pipeline.add("nvdsosd", "osd")
        pipeline.add("nvvideoconvert", "preview_convert")

        pipeline.add("nvv4l2h264enc", "encoder", {
            "bitrate": self._preview_bitrate,
            "preset-level": 1,
            "iframeinterval": 30,
            "maxperf-enable": 1,
        })

        pipeline.add("rtph264pay", "rtppay", {"pt": 96})

        pipeline.add("udpsink", "preview_udpsink", {
            "host": "127.0.0.1",
            "port": self._udpsink_port,
            "sync": 0,
            "async": 0,
        })

        pipeline.link(("tee", "queue_preview"), ("src_%u", ""))
        pipeline.link(
            "queue_preview", "tiler", "osd", "preview_convert",
            "encoder", "rtppay", "preview_udpsink",
        )

        comp.tiler_element = pipeline["tiler"]
