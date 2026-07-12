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
from .utils.validate_video import probe_video

VIDEO_STREAM_NAME = "video"

VIDEO_TOPOLOGY_DOC = """
    Inference chain::

        src → mux → pgie → tracker → analyzer → osd → nvvidconv
            → encoder → h264parse → mp4mux → filesink
"""


class DetVideoGenerator(DeepstreamGenerator):
    GENERATOR = "DetVideoGenerator"

    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    TRACKER_CONFIG_NAME = "nvtracker.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    PARAMS_NAME = "params.yml"

    f"""Generate YOLO detection video pipeline YAML.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        tracker: dict | None = None,
        interval: int = 0,
    ) -> None:
        self.input = Path(input).expanduser().resolve()
        self.output = Path(output).expanduser().resolve()
        self.analyzer = analyzer
        self.tracker = tracker
        self.pgie = pgie
        self.interval = interval

        super().__init__()

        self.init_input()
        self.init_pgie()
        self.init_nvdsanalytics()
        self.init_nvtracker()
        self.init_params()
        self.init_pipeline()

    def init_input(self) -> None:
        video_info = probe_video(self.input)
        self.width = video_info["width"]
        self.height = video_info["height"]
        self.fps = video_info["fps"]
        self.runtime_batch_size = 1
        self.output.parent.mkdir(parents=True, exist_ok=True)

    def init_params(self) -> None:
        self.params_yml = {}
        self.params_yml["generator"] = self.GENERATOR
        self.params_yml["input"] = str(self.input)
        self.params_yml["output"] = str(self.output)
        self.params_yml["pgie"] = self.pgie
        self.params_yml["analyzer"] = self.analyzer
        self.params_yml["tracker"] = self.tracker
        self.params_yml["interval"] = self.interval

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline

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
        if self.analyzer is not None:
            parser = NvdsanalyticsParser(
                self.analyzer["streams"],
                self.analyzer["template"],
            )
            parser.validate([VIDEO_STREAM_NAME], self.pgie_config_parser.class_ids)
            config = parser.build([VIDEO_STREAM_NAME], self.width, self.height)
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
        params = dict(self.params_yml)
        params["config_save_dir"] = str(config_save_dir)
        with open(params_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(params, handle, sort_keys=False, default_flow_style=False)

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
        }

    def add(self) -> None:
        self._append_node(
            "nvurisrcbin",
            "src",
            self._add_nvurisrcbin(
                self.file_uri(self.input),
                disable_audio=True,
            ),
        )
        self._append_node(
            "nvstreammux",
            "mux",
            self._add_nvstreammux(
                batch_size=1,
                width=self.width,
                height=self.height,
                live_source=False,
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
        gpu_id = self.pgie_generator.gpu_id
        self._append_node(
            "nvosdbin",
            "osd",
            self._add_nvosdbin(**self.osd_kwargs(gpu_id)),
        )
        self._append_node(
            "nvvideoconvert",
            "nvvidconv",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node(
            "nvv4l2h264enc",
            "encoder",
            self._add_nvv4l2h264enc(
                bitrate=4_000_000,
                iframeinterval=self.fps,
                preset_id=1,
                gpu_id=gpu_id,
            ),
        )
        self._append_node("h264parse", "h264parse", self._add_h264parse())
        self._append_node("mp4mux", "mp4mux", self._add_mp4mux())
        self._append_node(
            "filesink",
            "sink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "src": "mux",
            "mux": "pgie",
        }
        inference_tail = "pgie"
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        edges[inference_tail] = "analyzer"
        edges["analyzer"] = "osd"
        edges["osd"] = "nvvidconv"
        edges["nvvidconv"] = "encoder"
        edges["encoder"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "sink"
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def file_uri(path: str | Path) -> str:
        return Path(path).expanduser().resolve().as_uri()
