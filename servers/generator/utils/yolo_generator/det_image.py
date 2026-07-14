import copy
import shutil
from pathlib import Path

import yaml
from PIL import Image

from .base import DeepstreamGenerator
from ..subelement_generator import PgieGenerator
from .utils import YoloDet
from .utils.pgie_parser import PgieParser

IMAGE_TOPOLOGY_DOC = """
    Inference chain::

        src → mux → pgie → osd → nvvidconv → jpegenc → filesink
"""


class DetImageGenerator(DeepstreamGenerator):
    GENERATOR = "DetImageGenerator"

    PIPELINE_CONFIG_NAME = "pipeline.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    PARAMS_NAME = "params.yml"

    f"""Generate YOLO detection image pipeline YAML.

    Reads ``input`` image via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        pgie: dict,
        interval: int = 0,
    ) -> None:
        self.input = Path(input).expanduser().resolve()
        self.output = Path(output).expanduser().resolve()
        self.pgie = pgie
        self.interval = interval

        super().__init__()

        self.init_input()
        self.init_pgie()
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
        self.params_yml["interval"] = self.interval

    def init_pipeline(self) -> None:
        self.add()
        self.link()
        self.pipeline_yml = self.pipeline

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

    def apply_save_paths(self, config_save_dir: Path) -> None:
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "pgie":
                properties["config-file-path"] = str(
                    config_save_dir / self.PGIE_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        params_save_path = config_save_dir / self.PARAMS_NAME
        self.apply_save_paths(config_save_dir)
        shutil.copy2(
            self.pgie_config_parser.meta_path,
            config_save_dir / self.pgie_config_parser.meta_path.name,
        )
        with open(pgie_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.pgie_yml, handle, sort_keys=False, default_flow_style=False)
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
                num_buffers=1,
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
        self._append_node("nvjpegenc", "jpegenc", self._add_nvjpegenc())
        self._append_node(
            "filesink",
            "sink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "src": "mux",
            "mux": "pgie",
            "pgie": "osd",
            "osd": "nvvidconv",
            "nvvidconv": "jpegenc",
            "jpegenc": "sink",
        }
        self.pipeline["deepstream"]["edges"] = edges

    @staticmethod
    def file_uri(path: str | Path) -> str:
        return Path(path).expanduser().resolve().as_uri()
