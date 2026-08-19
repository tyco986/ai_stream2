import shutil
from pathlib import Path

import yaml
from PIL import Image

from ..subelement_generator.pipeline import PipelineGenerator
from ..subelement_generator.nvdsanalytics import NvdsanalyticsGenerator
from ..subelement_generator.pgie import PgieGenerator
from ..subelement_generator.utils.nvdsanalytics_parser import NvdsanalyticsParser
from ..subelement_generator.utils.pgie_parser import PgieParser

IMAGE_STREAM_NAME = "image"

IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → nvdsanalytics → nvosdbin → nvvideoconvert → nvjpegenc → filesink
"""


class BaseImageGenerator(PipelineGenerator):
    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    PGIE_META_NAME = "pgie_meta.json"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    PARAMS_NAME = "params.yml"
    SINK_PATH_CONFIG_NAME = "sink_path.yml"
    SINK_PATH_TEMPLATES = {
        "filesink": [
            "nvurisrcbin",
            "nvstreammux",
            "pgie",
            "nvdsanalytics",
            "nvosdbin",
            "nvvideoconvert",
            "nvjpegenc",
            "filesink",
        ],
    }

    f"""Generate YOLO image pipeline YAML.

    Reads ``input`` image via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
    ) -> None:
        self.input = Path(input).expanduser().resolve()
        self.output = Path(output).expanduser().resolve()
        self.analyzer = analyzer
        self.pgie = pgie

        super().__init__()

        self.init_input()
        self.init_pgie()
        self.init_nvdsanalytics()
        self.init_params()
        self.init_pipeline()

    def init_input(self) -> None:
        assert self.input.is_file(), f"input image not found: {self.input}"
        with Image.open(self.input) as image:
            self.width, self.height = image.size
        self.runtime_batch_size = 1
        self.output.parent.mkdir(parents=True, exist_ok=True)

    def init_params(self) -> None:
        self.params_yml = {}
        self.params_yml["generator"] = self.GENERATOR
        self.params_yml["input"] = str(self.input)
        self.params_yml["output"] = str(self.output)
        self.params_yml["pgie"] = self.pgie
        self.params_yml["analyzer"] = self.analyzer

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline

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
            template = dict(self.analyzer["template"])
            line_crossing = template.get("line_crossing")
            if line_crossing is not None:
                assert int(line_crossing.get("enable", 0)) == 0, (
                    "line_crossing enable must be 0 for image pipelines"
                )
                del template["line_crossing"]
            direction_detection = template.get("direction_detection")
            if direction_detection is not None:
                assert int(direction_detection.get("enable", 0)) == 0, (
                    "direction_detection enable must be 0 for image pipelines"
                )
                del template["direction_detection"]
            parser = NvdsanalyticsParser(
                self.analyzer["streams"],
                template,
                self.analyzer.get("osd_mode", 0),
            )
            parser.validate([IMAGE_STREAM_NAME], self.pgie_config_parser.class_ids)
            config = parser.build([IMAGE_STREAM_NAME], self.width, self.height)
            self.nvdsanalytics_yml = NvdsanalyticsGenerator(config).config

    def apply_save_paths(self, config_save_dir: Path) -> None:
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "pgie":
                properties["config-file-path"] = str(
                    config_save_dir / self.PGIE_CONFIG_NAME
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
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        nvdsanalytics_save_path = config_save_dir / self.ANALYTICS_CONFIG_NAME
        params_save_path = config_save_dir / self.PARAMS_NAME
        self.apply_save_paths(config_save_dir)
        shutil.copy2(
            self.pgie_config_parser.meta_path,
            config_save_dir / self.PGIE_META_NAME,
        )
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
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
        self.write_sink_path(config_save_dir)
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
            "nvurisrcbin",
            self._add_nvurisrcbin(
                self.file_uri(self.input),
                disable_audio=True,
                num_buffers=1,
            ),
        )
        self._append_node(
            "nvstreammux",
            "nvstreammux",
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
        self._append_node(
            "nvdsanalytics",
            "nvdsanalytics",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        gpu_id = self.pgie_generator.gpu_id
        self._append_node(
            "nvosdbin",
            "nvosdbin",
            self._add_nvosdbin(**self.osd_kwargs(gpu_id)),
        )
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert",
            self._add_nvvideoconvert(gpu_id=gpu_id),
        )
        self._append_node("nvjpegenc", "nvjpegenc", self._add_nvjpegenc())
        self._append_node(
            "filesink",
            "filesink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "nvurisrcbin": "nvstreammux",
            "nvstreammux": "pgie",
            "pgie": "nvdsanalytics",
            "nvdsanalytics": "nvosdbin",
            "nvosdbin": "nvvideoconvert",
            "nvvideoconvert": "nvjpegenc",
            "nvjpegenc": "filesink",
        }
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def file_uri(path: str | Path) -> str:
        return Path(path).expanduser().resolve().as_uri()
