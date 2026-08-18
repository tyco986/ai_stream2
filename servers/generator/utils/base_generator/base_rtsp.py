import shutil
from pathlib import Path

import yaml

from ..subelement_generator.pipeline import PipelineGenerator, TRACKER_LL_LIB
from ..subelement_generator.nvdsanalytics import NvdsanalyticsGenerator
from ..subelement_generator.nvtracker import NvtrackerGenerator
from ..subelement_generator.pgie import PgieGenerator
from ..subelement_generator.utils.pgie_parser import PgieParser
from ..subelement_generator.utils.nvdsanalytics_parser import NvdsanalyticsParser
from ..subelement_generator.utils.nvtracker_parser import NvtrackerParser

RTSP_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin{N} → nvstreammux → nvinfer → nvtracker → nvdsanalytics → nvstreamdemux
              → queue_demux{N} → nvvideoconvert{N} → fakesink{N}

    Python (not in pipeline.yml)::

        attach(nvdsanalytics, Probe)   # logger → drawer → messager
"""


class BaseRTSPGenerator(PipelineGenerator):
    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PAD_LINKS_CONFIG_NAME = "pad_links.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    TRACKER_CONFIG_NAME = "nvtracker.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    PARAMS_NAME = "params.yml"
    SINK_PATH_CONFIG_NAME = "sink_path.yml"
    SINK_PATH_TEMPLATES = {
        "fakesink{index}": [
            "nvurisrcbin{index}",
            "nvstreammux",
            "nvinfer",
            "nvtracker",
            "nvdsanalytics",
            "nvstreamdemux",
            "queue_demux{index}",
            "nvvideoconvert{index}",
            "fakesink{index}",
        ],
    }

    f"""Generate YOLO RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    DeepStream attaches probe on ``nvvideoconvert{{N}}`` for drawer / logger / messager.
    {RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        analyzer: dict | None,
        pgie: dict,
        tracker: dict | None = None,
    ) -> None:
        self.streams = streams
        self.analyzer = analyzer
        self.tracker = tracker
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

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline
        self.pad_links_yml = self.pad_links

    def init_pgie(self) -> None:
        self.pgie = {
            "model_dir": self.pgie["model_dir"],
            "class_attrs": self.pgie["class_attrs"],
            "interval": int(self.pgie.get("interval", 0)),
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

    def apply_save_paths(self, config_save_dir: Path) -> None:
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "nvinfer":
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


    def build_sink_paths(self) -> dict[str, list[str]]:
        templates = self.SINK_PATH_TEMPLATES
        indexed = any("{index}" in key for key in templates)
        sink_paths = {}
        if indexed:
            for index in range(len(self.streams)):
                for key_template, path_template in templates.items():
                    sink_paths[key_template.format(index=index)] = [
                        part.format(index=index) for part in path_template
                    ]
        else:
            sink_paths = {key: list(path) for key, path in templates.items()}
        return sink_paths

    def write_sink_path(self, config_save_dir: Path | str) -> None:
        path = Path(config_save_dir) / self.SINK_PATH_CONFIG_NAME
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.build_sink_paths(),
                handle,
                sort_keys=False,
                default_flow_style=False,
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
        self.write_sink_path(config_save_dir)
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
            "nvinfer",
            self._add_nvinfer(
                config_file_path=self.PGIE_CONFIG_NAME,
                batch_size=self.pgie_generator.batch_size,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        if self.enable_nvtracker:
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
        self._append_node("nvstreamdemux", "nvstreamdemux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "fakesink",
                f"fakesink{index}",
                self._add_fakesink(sync=False, async_=False),
            )

    def link(self) -> None:
        self.pad_links = {"nvstreamdemux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "nvinfer"
        inference_tail = "nvinfer"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = "nvstreamdemux"
        for index in range(len(self.streams)):
            self.pad_links["nvstreamdemux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvideoconvert{index}"
            edges[f"nvvideoconvert{index}"] = f"fakesink{index}"
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def visualized_sink_uri(source_uri: str) -> str:
        return f"{source_uri}_ds"
