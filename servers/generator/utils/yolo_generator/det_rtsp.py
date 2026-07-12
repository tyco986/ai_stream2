import copy
import shutil
from pathlib import Path

import yaml

from .base import (
    DeepstreamGenerator,
    TRACKER_LL_LIB,
    align_tracker_height,
)
from ..subelement_generator import (
    NvdsanalyticsGenerator,
    NvtrackerGenerator,
    PgieGenerator,
)
from ..subelement_generator.nvdsanalytics import nvdsanalytics_default_config
from .utils import YoloDet
from .utils.pgie_parser import PgieParser
from .utils.nvdsanalytics_parser import NvdsanalyticsParser
from .utils.nvtracker_parser import validate_tracker

RTSP_TOPOLOGY_DOC = """
    Batch inference chain::

        src{N} → mux → pgie → tracker → analyzer → demux

    Per-stream capture chain (cam{N})::

        demux → queue_demux{N} → nvvidconv{N} → tee_raw{N}
              ─┬→ queue_raw{N} → appsink_raw{N}     # raw frame, PNG on alert
              └→ osd{N} → queue_vis{N} → appsink_vis{N}   # osd frame, JPEG on alert

    Tee purpose (per stream)::

        tee_raw{N}
            Position: after nvvidconv, before osd.
            Meaning: split one NVMM frame into two consumers without copying pixels —
            (1) appsink_raw for alert-time original-frame PNG capture;
            (2) osd for drawing bbox / label / mask onto the frame.
            queue_raw is leaky so slow PNG encode does not block osd.

    Python (not in pipeline.yml)::

        attach(analyzer, BaseProbe)   # logger → debouncer → drawer → messager
        attach(appsink_raw{N}, Receiver)   # encode PNG when pending_capture matches
        attach(appsink_vis{N}, Receiver)   # encode JPEG when pending_capture matches
        event_queue → Kafka worker

    Topology overview::

        ┌────────┐   ┌─────┐   ┌────────┐   ┌───────────┐   ┌───────┐
        │ src0/1 │──►│ mux │──►│  pgie  │──►│  tracker  │──►│analyzer│──► demux
        └────────┘   └─────┘   └────────┘   └───────────┘   └───────┘
                                                                  │
                              ┌───────────────────────────────────┘
                              ▼
        queue → nvvidconv → tee_raw ─┬→ queue → appsink_raw   # original, pre-osd
                                     └→ osd → queue → appsink_vis   # post-osd
"""

VIS_RTSP_TOPOLOGY_DOC = """
    Batch inference chain::

        src{N} → mux → pgie → tracker → analyzer → demux

    Per-stream display / capture chain (cam{N})::

        demux → queue_demux{N} → nvvidconv{N} → tee_raw{N}
              ─┬→ queue_raw{N} → appsink_raw{N}     # raw frame, PNG on alert
              └→ osd{N} → tee_vis{N}
                    ─┬→ queue_vis{N} → appsink_vis{N}   # osd frame, JPEG on alert
                    └→ encoder{N} → h264parse{N} → rtspclientsink{N}

    Tee purpose (per stream)::

        tee_raw{N}
            Position: after nvvidconv, before osd.
            Meaning: split one NVMM frame into two consumers without copying pixels —
            (1) appsink_raw for alert-time original-frame PNG capture;
            (2) osd for drawing bbox / label / mask onto the frame.
            queue_raw is leaky so slow PNG encode does not block osd or RTSP.

        tee_vis{N}
            Position: after osd, before encoder.
            Meaning: split the osd-composited frame into two consumers —
            (1) appsink_vis for alert-time visualized JPEG capture;
            (2) encoder → rtspclientsink for continuous live preview.
            queue_vis is leaky so alert capture does not stall the preview encode path.

    Python (not in pipeline.yml)::

        attach(analyzer, BaseProbe)   # logger → debouncer → drawer → messager
        attach(appsink_raw{N}, Receiver)   # encode PNG when pending_capture matches
        attach(appsink_vis{N}, Receiver)   # encode JPEG when pending_capture matches
        event_queue → Kafka worker

    Topology overview::

        ┌────────┐   ┌─────┐   ┌────────┐   ┌───────────┐   ┌───────┐
        │ src0/1 │──►│ mux │──►│  pgie  │──►│  tracker  │──►│analyzer│──► demux
        └────────┘   └─────┘   └────────┘   └───────────┘   └───────┘
                                                                  │
                              ┌───────────────────────────────────┘
                              ▼
        queue → nvvidconv → tee_raw ─┬→ queue → appsink_raw   # original, pre-osd
                                     └→ osd → tee_vis ─┬→ queue → appsink_vis   # post-osd
                                                       └→ encoder → rtsp sink   # preview
"""


class DetRTSPGenerator(DeepstreamGenerator):
    GENERATOR = "DetRTSPGenerator"

    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PAD_LINKS_CONFIG_NAME = "pad_links.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    TRACKER_CONFIG_NAME = "nvtracker.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    PARAMS_NAME = "params.yml"

    f"""Generate YOLO detection RTSP pipeline for event alert + probe-side Kafka.

    Set ``analyzer=None`` to keep nvdsanalytics inserted with master switch off.
    Set ``tracker=None`` to skip nvtracker. Pass ``tracker={{"class_id": ...}}`` to insert nvtracker. Does not insert ``nvmsgconv`` / ``nvmsgbroker`` or RTSP preview sink;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        analyzer: dict | None,
        pgie: dict,
        tracker: dict | None = None,
        interval: int = 0,
    ) -> None:
        self.streams = streams
        self.analyzer = analyzer
        self.tracker = tracker
        self.interval = interval
        self.pgie = pgie

        super().__init__()

        self.init_streams()
        self.init_pgie()
        self.init_nvdsanalytics()
        self.init_nvtracker()
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
        self.params_yml["streams"] = self.streams
        self.params_yml["generator"] = self.GENERATOR
        self.params_yml["pgie"] = self.pgie
        self.params_yml["analyzer"] = self.analyzer
        self.params_yml["tracker"] = self.tracker
        self.params_yml["interval"] = self.interval

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline
        self.pad_links_yml = self.pad_links

    def tracker_dimensions(self) -> tuple[int, int]:
        return self.width, align_tracker_height(self.height)

    def init_pgie(self) -> None:
        class_on = self.pgie.get("class_on")
        if class_on is not None:
            assert len(class_on) == len(set(class_on)), (
                "pgie class_on contains duplicate class ids"
            )
            class_on = list(set(class_on))
        self.pgie = {
            "model_dir": self.pgie["model_dir"],
            "class_attr": self.pgie["class_attr"],
            "class_on": class_on,
        }
        self.pgie_config_parser = PgieParser(
            self.pgie["model_dir"],
            self.runtime_batch_size,
            self.pgie["class_attr"],
            self.pgie["class_on"],
            self.interval,
        )
        self.pgie_generator = PgieGenerator(**self.pgie_config_parser.build())
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def init_nvdsanalytics(self) -> None:
        pipeline_stream_names = list(self.streams.keys())
        if self.analyzer is not None:
            parser = NvdsanalyticsParser(
                self.analyzer["streams"],
                self.analyzer["template"],
            )
            parser.validate(pipeline_stream_names, self.pgie_config_parser.class_ids)
            config = parser.build(pipeline_stream_names, self.width, self.height)
            self.nvdsanalytics_yml = NvdsanalyticsGenerator(config).config
        else:
            config = copy.deepcopy(nvdsanalytics_default_config)
            config["property"]["config-width"] = self.width
            config["property"]["config-height"] = self.height
            self.nvdsanalytics_yml = NvdsanalyticsGenerator(config).config

    def init_nvtracker(self) -> None:
        self.nvtracker_generator = None
        self.nvtracker_yml = None
        self.enable_nvtracker = validate_tracker(
            self.tracker,
            self.pgie_config_parser.class_ids,
        )
        if self.enable_nvtracker:
            self.nvtracker_generator = NvtrackerGenerator()
            self.nvtracker_yml = self.nvtracker_generator.config

    def apply_save_paths(self, config_save_dir: Path) -> None:
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "pgie":
                properties["config-file-path"] = str(
                    config_save_dir / self.PGIE_CONFIG_NAME
                )
            if name == "tracker":
                properties["ll-config-file"] = str(
                    config_save_dir / self.TRACKER_CONFIG_NAME
                )
            if name == "analyzer":
                properties["config-file"] = str(
                    config_save_dir / self.ANALYTICS_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pad_links_save_path = config_save_dir / self.PAD_LINKS_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        nvtracker_save_path = config_save_dir / self.TRACKER_CONFIG_NAME
        nvdsanalytics_save_path = config_save_dir / self.ANALYTICS_CONFIG_NAME
        params_save_path = config_save_dir / self.PARAMS_NAME
        self.apply_save_paths(config_save_dir)
        shutil.copy2(
            self.pgie_config_parser.meta_path,
            config_save_dir / self.pgie_config_parser.meta_path.name,
        )
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
        if self.enable_nvtracker:
            with open(nvtracker_save_path, "w", encoding="utf-8") as handle:
                yaml.safe_dump(
                    self.nvtracker_yml, handle, sort_keys=False, default_flow_style=False
                )
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
        with open(pad_links_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.pad_links_yml, handle, sort_keys=False, default_flow_style=False
            )
        params = dict(self.params_yml)
        params["config_save_dir"] = str(config_save_dir)
        with open(params_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(params, handle, sort_keys=False, default_flow_style=False)

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
        }

    def add(self) -> None:
        for index, name in enumerate(self.streams):
            self._append_node(
                "nvurisrcbin",
                f"src{index}",
                self._add_nvurisrcbin(self.streams[name]["url"], disable_audio=True),
            )
        self._append_node(
            "nvstreammux",
            "mux",
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
            tracker_width, tracker_height = self.tracker_dimensions()
            self._append_node(
                "nvtracker",
                "tracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    self.TRACKER_CONFIG_NAME,
                    tracker_width=tracker_width,
                    tracker_height=tracker_height,
                    gpu_id=self.pgie_generator.gpu_id,
                ),
            )
        self._append_node(
            "nvdsanalytics",
            "analyzer",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node("nvstreamdemux", "demux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node("tee", f"tee_raw{index}", self._add_tee())
            self._append_node("queue", f"queue_raw{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_raw{index}",
                self._add_appsink(),
            )
            self._append_node(
                "nvosdbin",
                f"osd{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_vis{index}",
                self._add_appsink(),
            )

    def link(self) -> None:
        self.pad_links = {"demux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"src{index}"] = "mux"
        edges["mux"] = "pgie"
        inference_tail = "pgie"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "demux"
        for index in range(len(self.streams)):
            self.pad_links["demux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvidconv{index}"
            edges[f"nvvidconv{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"osd{index}"]
            edges[f"queue_raw{index}"] = f"appsink_raw{index}"
            edges[f"osd{index}"] = f"queue_vis{index}"
            edges[f"queue_vis{index}"] = f"appsink_vis{index}"
        self.pipeline["deepstream"]["edges"] = edges


class DetVisRTSPGenerator(DetRTSPGenerator):
    GENERATOR = "DetVisRTSPGenerator"

    f"""Generate YOLO detection RTSP pipeline for event alert + probe-side Kafka + live preview.

    Requires ``enable_visualized_rtsp=True``.
    Set ``analyzer=None`` to keep nvdsanalytics inserted with master switch off.
    Set ``tracker=None`` to skip nvtracker. Pass ``tracker={{"class_id": ...}}`` to insert nvtracker. Does not insert ``nvmsgconv`` / ``nvmsgbroker``;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        enable_visualized_rtsp: bool,
        analyzer: dict | None,
        pgie: dict,
        tracker: dict | None = None,
        interval: int = 0,
    ) -> None:
        self.enable_visualized_rtsp = enable_visualized_rtsp
        assert enable_visualized_rtsp, "enable_visualized_rtsp must be True"
        super().__init__(
            streams=streams,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
            interval=interval,
        )

    def add(self) -> None:
        for index, name in enumerate(self.streams):
            self._append_node(
                "nvurisrcbin",
                f"src{index}",
                self._add_nvurisrcbin(self.streams[name]["url"], disable_audio=True),
            )
        self._append_node(
            "nvstreammux",
            "mux",
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
            tracker_width, tracker_height = self.tracker_dimensions()
            self._append_node(
                "nvtracker",
                "tracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    self.TRACKER_CONFIG_NAME,
                    tracker_width=tracker_width,
                    tracker_height=tracker_height,
                    gpu_id=self.pgie_generator.gpu_id,
                ),
            )
        self._append_node(
            "nvdsanalytics",
            "analyzer",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node("nvstreamdemux", "demux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index, name in enumerate(self.streams):
            sink_uri = self.visualized_sink_uri(self.streams[name]["url"])
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node("tee", f"tee_raw{index}", self._add_tee())
            self._append_node("queue", f"queue_raw{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_raw{index}",
                self._add_appsink(),
            )
            self._append_node(
                "nvosdbin",
                f"osd{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("tee", f"tee_vis{index}", self._add_tee())
            self._append_node("queue", f"queue_vis{index}", self._add_queue())
            self._append_node(
                "appsink",
                f"appsink_vis{index}",
                self._add_appsink(),
            )
            self._append_node(
                "nvv4l2h264enc",
                f"encoder{index}",
                self._add_nvv4l2h264enc(
                    bitrate=4_000_000,
                    iframeinterval=self.fps,
                    preset_id=1,
                    gpu_id=gpu_id,
                ),
            )
            self._append_node("h264parse", f"h264parse{index}", self._add_h264parse())
            self._append_node(
                "rtspclientsink",
                f"sink{index}",
                self._add_rtspclientsink(location=sink_uri, sync=False, async_=False),
            )

    def link(self) -> None:
        self.pad_links = {"demux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"src{index}"] = "mux"
        edges["mux"] = "pgie"
        inference_tail = "pgie"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "demux"
        for index in range(len(self.streams)):
            self.pad_links["demux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvidconv{index}"
            edges[f"nvvidconv{index}"] = f"tee_raw{index}"
            edges[f"tee_raw{index}"] = [f"queue_raw{index}", f"osd{index}"]
            edges[f"queue_raw{index}"] = f"appsink_raw{index}"
            edges[f"osd{index}"] = f"tee_vis{index}"
            edges[f"tee_vis{index}"] = [f"queue_vis{index}", f"encoder{index}"]
            edges[f"queue_vis{index}"] = f"appsink_vis{index}"
            edges[f"encoder{index}"] = f"h264parse{index}"
            edges[f"h264parse{index}"] = f"sink{index}"
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def visualized_sink_uri(source_uri: str) -> str:
        return f"{source_uri}_ds"
