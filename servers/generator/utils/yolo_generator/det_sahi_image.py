import copy
import json
import shutil
from pathlib import Path

import yaml

from .base import DeepstreamGenerator
from ..subelement_generator import NvsahipreprocessGenerator
from .det_image import DetImageGenerator, IMAGE_TOPOLOGY_DOC
from .utils import YoloDetSahi, get_sahi_box, get_sahi_preview

SAHI_IMAGE_TOPOLOGY_DOC = """
    Inference chain::

        src → mux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → osd → nvvidconv → jpegenc → filesink

    ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
"""


class DetSahiImageGenerator(DetImageGenerator):
    GENERATOR = "DetSahiImageGenerator"
    SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini"

    f"""Generate YOLO SAHI detection image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {IMAGE_TOPOLOGY_DOC}
    {SAHI_IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        pgie: dict,
        sahi: dict,
        interval: int = 0,
    ) -> None:
        self.sahi = sahi
        self.input = Path(input).expanduser().resolve()
        self.output = Path(output).expanduser().resolve()
        self.pgie = pgie
        self.interval = interval
        DeepstreamGenerator.__init__(self)
        self.init_input()
        self.init_pgie()
        self.init_sahi()
        self.init_params()
        self.init_pipeline()

    def init_input(self) -> None:
        super().init_input()
        self.mux_batch_size = self.runtime_batch_size
        sahi = self.sahi["nvsahipreprocess"]
        slice_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )
        self.runtime_batch_size = int(slice_info["num"])

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloDetSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def init_sahi(self) -> None:
        meta = self.pgie_config_parser.meta
        input_shape = meta["input_tensor_shape"]
        network_input_shape = ";".join(
            str(value) for value in [self.runtime_batch_size, *input_shape[1:]]
        )
        self.nvsahipreprocess_generator = NvsahipreprocessGenerator(
            network_input_shape=network_input_shape,
            target_unique_ids=self.pgie_generator.config["property"]["gie-unique-id"],
            tensor_data_type=0,
            tensor_name=meta["input_tensor_name"],
        )
        self.nvsahipreprocess_yml = self.nvsahipreprocess_generator.config

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["sahi"] = self.sahi

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "sahi_preprocess":
                properties["config-file"] = str(
                    config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        config_save_dir = Path(config_save_dir)
        pipeline_save_path = config_save_dir / self.PIPELINE_CONFIG_NAME
        pgie_save_path = config_save_dir / self.PGIE_CONFIG_NAME
        params_save_path = config_save_dir / self.PARAMS_NAME
        sahi_preprocess_save_path = config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
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
        self.nvsahipreprocess_generator.write(sahi_preprocess_save_path)
        sahi = self.sahi["nvsahipreprocess"]
        sahi_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )
        sahi_show = get_sahi_preview(sahi_info)
        sahi_show.save(config_save_dir / "sahi_slice_preview.jpg")
        with open(config_save_dir / "sahi_slice_info.json", "w", encoding="utf-8") as handle:
            json.dump(sahi_info, handle)

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
                batch_size=self.mux_batch_size,
                width=self.width,
                height=self.height,
                live_source=False,
                enable_padding=False,
                batched_push_timeout=40000,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        sahi = self.sahi["nvsahipreprocess"]
        postprocess = self.sahi["nvsahipostprocess"]
        self._append_node(
            "nvsahipreprocess",
            "sahi_preprocess",
            self._add_nvsahipreprocess(
                self.SAHI_PREPROCESS_CONFIG_NAME,
                slice_width=sahi["slice_width"],
                slice_height=sahi["slice_height"],
                overlap_width_ratio=sahi["overlap_width_ratio"],
                overlap_height_ratio=sahi["overlap_height_ratio"],
                enable_full_frame=True,
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
                input_tensor_meta=True,
            ),
        )
        self._append_node("queue", "queue_sahi", self._add_queue())
        self._append_node(
            "nvsahipostprocess",
            "sahi_postprocess",
            self._add_nvsahipostprocess(
                gie_ids=str(self.pgie_generator.config["property"]["gie-unique-id"]),
                match_metric=1,
                match_threshold=postprocess["match_threshold"],
                class_agnostic=False,
                enable_merge=True,
                two_phase_nmm=True,
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
            "mux": "sahi_preprocess",
            "sahi_preprocess": "pgie",
            "pgie": "queue_sahi",
            "queue_sahi": "sahi_postprocess",
            "sahi_postprocess": "osd",
            "osd": "nvvidconv",
            "nvvidconv": "jpegenc",
            "jpegenc": "sink",
        }
        self.pipeline["deepstream"]["edges"] = edges
