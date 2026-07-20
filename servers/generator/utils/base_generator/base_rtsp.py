import shutil
from pathlib import Path

import yaml

from ..subelement_generator.pipeline import PipelineGenerator, TRACKER_LL_LIB
from ..subelement_generator import (
    NvdsanalyticsGenerator,
    NvtrackerGenerator,
    PgieGenerator,
)
from ..subelement_generator.utils.pgie_parser import PgieParser
from ..subelement_generator.utils.nvdsanalytics_parser import NvdsanalyticsParser
from ..subelement_generator.utils.nvtracker_utils import align_tracker_dimension, format_operate_on_class_ids, validate_tracker

RTSP_TOPOLOGY_DOC = """
    Topology::

        src{N} → mux → pgie → tracker → analyzer → demux
              → queue_demux{N} → nvvidconv{N} → fakesink{N}

    Python (not in pipeline.yml)::

        attach(analyzer, Probe)   # logger → drawer → messager
"""


class BaseRTSPGenerator(PipelineGenerator):
    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PAD_LINKS_CONFIG_NAME = "pad_links.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    TRACKER_CONFIG_NAME = "nvtracker.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    PARAMS_NAME = "params.yml"

    f"""Generate YOLO RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    DeepStream attaches probe on ``nvvidconv{{N}}`` for drawer / logger / messager.
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

    def init_pgie(self) -> None:
        self.pgie = {
            "model_dir": self.pgie["model_dir"],
            "class_attrs": self.pgie["class_attrs"],
        }
        self.pgie_config_parser = PgieParser(
            self.pgie["model_dir"],
            self.runtime_batch_size,
            self.pgie["class_attrs"],
            self.interval,
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
        self.enable_nvtracker = validate_tracker(
            self.tracker,
            self.pgie_config_parser.class_ids,
        )
        if self.enable_nvtracker:
            self.nvtracker_generator = NvtrackerGenerator()
            self.nvtracker_yml = self.nvtracker_generator.config
            self.tracker_width = align_tracker_dimension(self.width)
            self.tracker_height = align_tracker_dimension(self.height)
            self.operate_on_class_ids = format_operate_on_class_ids(self.tracker)

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
            self._append_node(
                "nvtracker",
                "tracker",
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
            "analyzer",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node("nvstreamdemux", "demux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvidconv{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "fakesink",
                f"sink{index}",
                self._add_fakesink(sync=False, async_=False),
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
            edges[f"nvvidconv{index}"] = f"sink{index}"
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def visualized_sink_uri(source_uri: str) -> str:
        return f"{source_uri}_ds"
