from pathlib import Path

import yaml

from ..subelement_generator.kafka import (
    KAFKA_CONN_STR,
    KAFKA_PROTO_LIB,
    PROJECT_NAME,
    KafkaGenerator,
)
from ..subelement_generator.nvdsanalytics import NvdsanalyticsGenerator
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL, NvmsgconvGenerator
from ..subelement_generator.nvtracker import TRACKER_LL_LIB, NvtrackerGenerator
from ..subelement_generator.pgie import PgieGenerator
from ..subelement_generator.pipeline import PipelineGenerator
from ..subelement_generator.utils.pgie_parser import PgieParser
from ..subelement_generator.utils.nvdsanalytics_parser import NvdsanalyticsParser
from ..subelement_generator.utils.nvtracker_parser import NvtrackerParser
from .utils.event_pipeline import EventPipelineMixin
from .utils.generator_pipeline_map import GENERATOR_PIPELINE_MAP

RTSP_EVENT_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
              ─┬→ nvstreamdemux
              → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
                    ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → nvrawcapturer{N} → fakesink_raw{N}
                    └→ queue_osd{N} → nvvideoconvert_osd{N} → capsfilter_osd{N}(RGBA) → nvosdbin{N}
                          → nvpresencelogger{N} → queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → nvviscapturer{N} → fakesink_vis{N}

    Notes::

        No RTSP preview sink: capture ends at capturer + fakesink.
        ``nvvideoconvert_raw`` / ``nvvideoconvert_osd`` after ``tee_raw`` force independent NVMM
        buffers so ``nvosd`` in-place drawing cannot leak into the raw capture branch.
        Capture branches force ``format=RGB`` (``nvrawcapturer`` / ``nvviscapturer`` NVMM caps).

"""


class BaseEventRTSPGenerator(EventPipelineMixin, PipelineGenerator):
    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    TRACKER_CONFIG_NAME = "nvtracker.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    MSGCONV_CONFIG_NAME = "nvmsgconv.yml"
    KAFKA_CONFIG_NAME = "kafka.txt"
    PARAMS_CONFIG_NAME = "params.yml"

    f"""Generate YOLO RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {RTSP_EVENT_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        streams: dict[str, dict],
        analyzer: dict | None,
        pgie: dict,
        event_coder: dict,
        tracker: dict | None = None,
        logger: dict | None = None,
        drawer: dict | None = None,
        capturer: dict | None = None,
    ) -> None:
        self.pipeline_name = pipeline_name
        self.streams = streams
        self.analyzer = analyzer
        self.tracker = tracker
        self.pgie = pgie
        self.logger = {"interval": 0}
        if logger is not None:
            self.logger.update(logger)
        self.logger["root"] = f"/root/logs/deepstream/{pipeline_name}"
        self.drawer = drawer
        self.bind_event(event_coder, capturer)

        super().__init__()

        self.init_streams()
        self.init_pgie()
        self.init_nvdsanalytics()
        self.init_nvtracker()
        self.init_nvmsgconv()
        self.init_kafka()
        self.init_params()
        self.init_pipeline()

    def init_streams(self) -> None:
        assert self.streams, "streams cannot be empty"
        widths = set()
        heights = set()
        fps_values = set()
        for name, stream in self.streams.items():
            widths.add(int(stream["width"]))
            heights.add(int(stream["height"]))
            fps_values.add(int(stream["fps"]))
        assert len(widths) == 1, f"streams have inconsistent width: {widths}"
        assert len(heights) == 1, f"streams have inconsistent height: {heights}"
        assert len(fps_values) == 1, f"streams have inconsistent fps: {fps_values}"
        self.width = widths.pop()
        self.height = heights.pop()
        self.fps = fps_values.pop()
        self.runtime_batch_size = len(self.streams)

    def init_params(self) -> None:
        self.params_yml = {}
        self.params_yml["pipeline_name"] = self.pipeline_name
        self.params_yml["streams"] = self.streams
        self.params_yml["type"] = GENERATOR_PIPELINE_MAP[self.GENERATOR]
        self.params_yml["generator"] = self.GENERATOR
        self.params_yml["pgie"] = self.pgie
        self.params_yml["analyzer"] = self.analyzer
        self.params_yml["tracker"] = self.tracker
        self.params_yml["logger"] = self.logger
        self.params_yml["drawer"] = self.drawer
        self.params_yml["event_coder"] = self.event_coder
        self.params_yml["capturer"] = self.capturer

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline

    def init_pgie(self) -> None:
        self.pgie = {
            "model_dir": self.pgie["model_dir"],
            "class_attrs": self.pgie["class_attrs"],
            "interval": int(self.pgie.get("interval", 1)),
        }
        self.pgie_config_parser = PgieParser(
            self.pgie["model_dir"],
            self.runtime_batch_size,
            self.pgie["class_attrs"],
            self.pgie["interval"],
        )
        self.pgie_generator = PgieGenerator(**self.pgie_config_parser.build())
        self.apply_pgie_config()

    def apply_pgie_config(self) -> None:
        raise NotImplementedError

    def init_nvdsanalytics(self) -> None:
        self.enable_nvdsanalytics = self.analyzer is not None
        if not self.enable_nvdsanalytics:
            self.nvdsanalytics_yml = NvdsanalyticsGenerator().config
            self.nvdsanalytics_yml["property"]["config-width"] = self.width
            self.nvdsanalytics_yml["property"]["config-height"] = self.height
        else:
            parser = NvdsanalyticsParser(
                self.analyzer["streams"],
                self.analyzer["template"],
                self.analyzer.get("osd_mode", 0),
            )
            pipeline_stream_names = list(self.streams.keys())
            parser.validate(pipeline_stream_names, self.pgie_config_parser.class_ids)
            config = parser.build(pipeline_stream_names, self.width, self.height)
            self.nvdsanalytics_yml = NvdsanalyticsGenerator(config).config

    def init_nvtracker(self) -> None:
        self.nvtracker_generator = None
        self.nvtracker_yml = None
        parser = NvtrackerParser(self.pgie["interval"])
        self.enable_nvtracker = parser.validate(
            self.tracker,
            self.pgie_config_parser.class_ids,
        )
        if self.enable_nvtracker:
            self.nvtracker_generator = NvtrackerGenerator(
                maxShadowTrackingAge=parser.maxShadowTrackingAge,
                earlyTerminationAge=parser.earlyTerminationAge,
                probationAge=parser.probationAge,
            )
            self.nvtracker_yml = self.nvtracker_generator.config
            self.tracker_width = parser.align_tracker_dimension(self.width)
            self.tracker_height = parser.align_tracker_dimension(self.height)
            self.operate_on_class_ids = parser.format_operate_on_class_ids(self.tracker)

    def kafka_streams(self) -> list[str]:
        return list(self.streams)

    def init_nvmsgconv(self) -> None:
        self.nvmsgconv_generator = NvmsgconvGenerator(self.kafka_streams())
        self.nvmsgconv_yml = self.nvmsgconv_generator.config

    def init_kafka(self) -> None:
        self.kafka_generator = KafkaGenerator()
        self.kafka_yml = self.kafka_generator.config
        self.kafka_topic = f"{PROJECT_NAME}_{self.pipeline_name}"

    def apply_save_paths(self, config_save_dir: Path) -> None:
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "pgie":
                properties["config-file-path"] = str(
                    config_save_dir / self.PGIE_CONFIG_NAME
                )
            if name == "nvtracker":
                properties["ll-config-file"] = str(
                    config_save_dir / self.TRACKER_CONFIG_NAME
                )
            if name == "nvdsanalytics":
                properties["config-file"] = str(
                    config_save_dir / self.ANALYTICS_CONFIG_NAME
                )
            if name == "nvmsgconv":
                properties["config"] = str(config_save_dir / self.MSGCONV_CONFIG_NAME)
            if name == "nvmsgbroker":
                properties["config"] = str(config_save_dir / self.KAFKA_CONFIG_NAME)

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        nvtracker_save_path = config_save_dir / self.TRACKER_CONFIG_NAME
        nvdsanalytics_save_path = config_save_dir / self.ANALYTICS_CONFIG_NAME
        self.apply_save_paths(config_save_dir)
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
        if self.enable_nvtracker:
            with open(nvtracker_save_path, "w", encoding="utf-8") as handle:
                yaml.safe_dump(
                    self.nvtracker_yml, handle, sort_keys=False, default_flow_style=False
                )
        else:
            nvtracker_save_path.unlink(missing_ok=True)
        with open(nvdsanalytics_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.nvdsanalytics_yml,
                handle,
                sort_keys=False,
                default_flow_style=False,
            )
        with open(pipeline_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.pipeline_yml, handle, sort_keys=False, default_flow_style=False
            )
        with open(config_save_dir / self.MSGCONV_CONFIG_NAME, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.nvmsgconv_yml, handle, sort_keys=False, default_flow_style=False
            )
        (config_save_dir / self.KAFKA_CONFIG_NAME).write_text(
            self.kafka_yml, encoding="utf-8"
        )
        with open(config_save_dir / self.PARAMS_CONFIG_NAME, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.params_yml, handle, sort_keys=False, default_flow_style=False
            )

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
        }

    def nvdet_drawer_element(self) -> str:
        element = "nvdetfadedrawer"
        if self.enable_nvtracker:
            element = "nvdetfadedrawerwithtracker"
        return element

    def nvpose_drawer_element(self) -> str:
        element = "nvposefadedrawer"
        if self.enable_nvtracker:
            element = "nvposefadedrawerwithtracker"
        return element

    def nvseg_drawer_element(self) -> str:
        element = "nvsegfadedrawer"
        if self.enable_nvtracker:
            element = "nvsegfadedrawerwithtracker"
        return element

    def nvdet_drawer_properties(self, drawer: dict) -> dict:
        interval = int(drawer.get("interval", 0))
        fade_time = int(drawer.get("fade_time", 0))
        show_label = bool(drawer.get("show_label", False))
        properties = self._add_nvdetfadedrawer(interval, fade_time, show_label)
        if self.enable_nvtracker:
            properties = self._add_nvdetfadedrawerwithtracker(
                interval,
                fade_time,
                show_label,
                show_snap=bool(drawer.get("show_snap", True)),
            )
        return properties

    def nvpose_drawer_properties(self, drawer: dict) -> dict:
        interval = int(drawer.get("interval", 0))
        fade_time = int(drawer.get("fade_time", 0))
        show_label = bool(drawer.get("show_label", False))
        show_pose = bool(drawer.get("show_pose", True))
        pose_threshold = float(drawer.get("pose_threshold", 0.0))
        mode = drawer.get("mode", "coco17")
        properties = self._add_nvposefadedrawer(
            interval, fade_time, show_label, show_pose, pose_threshold, mode
        )
        if self.enable_nvtracker:
            properties = self._add_nvposefadedrawerwithtracker(
                interval,
                fade_time,
                show_label,
                show_pose,
                pose_threshold,
                mode,
                show_snap=bool(drawer.get("show_snap", True)),
            )
        return properties

    def nvseg_drawer_properties(self, drawer: dict) -> dict:
        interval = int(drawer.get("interval", 0))
        fade_time = int(drawer.get("fade_time", 0))
        show_label = bool(drawer.get("show_label", False))
        show_mask = bool(drawer.get("show_mask", True))
        properties = self._add_nvsegfadedrawer(
            interval, fade_time, show_label, show_mask
        )
        if self.enable_nvtracker:
            properties = self._add_nvsegfadedrawerwithtracker(
                interval,
                fade_time,
                show_label,
                show_mask,
                show_snap=bool(drawer.get("show_snap", True)),
            )
        return properties

    def visualized_sink_uri(self, source_uri: str) -> str:
        return f"{source_uri}/{self.kafka_topic}"
    def add(self) -> None:
        for index, name in enumerate(self.streams):
            self._append_node(
                "nvurisrcbin",
                f"nvurisrcbin{index}",
                self._add_nvurisrcbin(self.streams[name]["url"], disable_audio=True),
            )
        self._append_node(
            "nvstreammux",
            "nvstreammux",
            self._add_nvstreammux(
                batch_size=len(self.streams),
                width=self.width,
                height=self.height,
                live_source=True,
                enable_padding=False,
                batched_push_timeout=40000,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node(
            "nvinfer",
            "pgie",
            self._add_nvinfer(
                config_file_path=self.PGIE_CONFIG_NAME,
                batch_size=self.pgie_generator.batch_size,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        if self.enable_nvtracker:
            if self.drawer is not None:
                self._append_node(
                    "nvbboxsnapshot",
                    "nvbboxsnapshot",
                    self._add_nvbboxsnapshot(),
                )
            self._append_node(
                "nvtracker",
                "nvtracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    self.TRACKER_CONFIG_NAME,
                    tracker_width=self.tracker_width,
                    tracker_height=self.tracker_height,
                    gpu_id=self.pgie_generator.gpu_id,
                    operate_on_class_ids=self.operate_on_class_ids,
                ),
            )
        self._append_node(
            "nvdsanalytics",
            "nvdsanalytics",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self.append_event_coder()
        self._append_node("tee", "tee_msg", self._add_tee())
        self._append_node("queue", "queue_msg", self._add_queue())
        self._append_node(
            "nvmsgconv",
            "nvmsgconv",
            self._add_nvmsgconv(
                self.MSGCONV_CONFIG_NAME,
                payload_type=PAYLOAD_DEEPSTREAM_MINIMAL,
            ),
        )
        self._append_node(
            "nvmsgbroker",
            "nvmsgbroker",
            self._add_nvmsgbroker(
                KAFKA_PROTO_LIB,
                KAFKA_CONN_STR,
                self.kafka_topic,
                self.KAFKA_CONFIG_NAME,
                sync=False,
                async_=False,
            ),
        )
        self._append_node("nvstreamdemux", "nvstreamdemux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node("tee", f"tee_raw{index}", self._add_tee())
            self._append_node("queue", f"queue_raw{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert_raw{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"capsfilter_raw{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
            )
            self._append_node(
                "nvrawcapturer",
                f"nvrawcapturer{index}",
                self._add_nvrawcapturer(output_dir=self.capture_output_dir(), capture_codes=self.capturer_codes()),
            )
            self._append_node(
                "fakesink",
                f"fakesink_raw{index}",
                self._add_fakesink(sync=False, async_=False),
            )
            self._append_node("queue", f"queue_osd{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert_osd{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"capsfilter_osd{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGBA"),
            )
            if self.drawer is not None:
                drawer = self.drawer
                self._append_node(
                    self.nvdet_drawer_element(),
                    f"nvdetfadedrawer{index}",
                    self.nvdet_drawer_properties(drawer),
                )
            self._append_node(
                "nvosdbin",
                f"nvosdbin{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node(
                "nvpresencelogger",
                f"nvpresencelogger{index}",
                self._add_nvpresencelogger(
                    root=f"/root/logs/deepstream/{self.pipeline_name}",
                    interval=int(self.logger.get("interval", 0)),
                ),
            )
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert_vis{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "capsfilter",
                f"capsfilter_vis{index}",
                self._add_capsfilter("video/x-raw(memory:NVMM), format=RGB"),
            )
            self._append_node(
                "nvviscapturer",
                f"nvviscapturer{index}",
                self._add_nvviscapturer(output_dir=self.capture_output_dir(), capture_codes=self.capturer_codes()),
            )
            self._append_node(
                "fakesink",
                f"fakesink_vis{index}",
                self._add_fakesink(sync=False, async_=False),
            )

    def link(self) -> None:
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "pgie"
        inference_tail = "pgie"
        if self.enable_nvtracker:
            if self.drawer is not None:
                edges[inference_tail] = "nvbboxsnapshot"
                edges["nvbboxsnapshot"] = "nvtracker"
            else:
                edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = self.after_analytics()
        self.link_event_coder(edges)
        edges["tee_msg"] = ["nvstreamdemux", "queue_msg"]
        edges["queue_msg"] = "nvmsgconv"
        edges["nvmsgconv"] = "nvmsgbroker"
        for index in range(len(self.streams)):
            edges[f"queue_demux{index}"] = f"nvvideoconvert{index}"
            edges[f"nvvideoconvert{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"queue_osd{index}"]
            edges[f"queue_raw{index}"] = f"nvvideoconvert_raw{index}"
            edges[f"nvvideoconvert_raw{index}"] = f"capsfilter_raw{index}"
            edges[f"capsfilter_raw{index}"] = f"nvrawcapturer{index}"
            edges[f"nvrawcapturer{index}"] = f"fakesink_raw{index}"
            edges[f"queue_osd{index}"] = f"nvvideoconvert_osd{index}"
            edges[f"nvvideoconvert_osd{index}"] = f"capsfilter_osd{index}"
            osd_prev = f"nvosdbin{index}"
            if self.drawer is not None:
                osd_prev = f"nvdetfadedrawer{index}"
            edges[f"capsfilter_osd{index}"] = osd_prev
            if self.drawer is not None:
                edges[f"nvdetfadedrawer{index}"] = f"nvosdbin{index}"
            edges[f"nvosdbin{index}"] = f"nvpresencelogger{index}"
            edges[f"nvpresencelogger{index}"] = f"queue_vis{index}"
            edges[f"queue_vis{index}"] = f"nvvideoconvert_vis{index}"
            edges[f"nvvideoconvert_vis{index}"] = f"capsfilter_vis{index}"
            edges[f"capsfilter_vis{index}"] = f"nvviscapturer{index}"
            edges[f"nvviscapturer{index}"] = f"fakesink_vis{index}"
        self.pipeline["deepstream"]["edges"] = edges
