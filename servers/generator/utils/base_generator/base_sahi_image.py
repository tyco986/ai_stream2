import json
from pathlib import Path

from ..subelement_generator.kafka import KAFKA_CONN_STR, KAFKA_PROTO_LIB
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL
from ..subelement_generator.nvsahipreprocess import NvsahipreprocessGenerator
from .base_image import BaseImageGenerator
from ..subelement_generator.utils.sahi import get_sahi_box, get_sahi_preview

SAHI_IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → nvdsanalytics → tee_msg
              ─┬→ nvosdbin → nvvideoconvert → nvdetlogger → nvjpegenc → filesink
              └→ queue_msg → nvmsgconv → nvmsgbroker

    Notes::

        ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
"""


class BaseSahiImageGenerator(BaseImageGenerator):
    SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini"
    SAHI_POSTPROCESS = "nvsahipostprocess"

    f"""Generate YOLO SAHI image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI inference with OSD, and writes the
    annotated result to ``output``.
    {SAHI_IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sahi: dict,
        logger: dict | None = None,
        drawer: dict | None = None,
        event_coder: dict | None = None,
    ) -> None:
        self.sahi = sahi
        super().__init__(
            pipeline_name=pipeline_name,
            input=input,
            output=output,
            analyzer=analyzer,
            pgie=pgie,
            logger=logger,
            drawer=drawer,
            event_coder=event_coder,
        )

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
            enable_full_frame=sahi.get("enable_full_frame", True),
        )
        self.runtime_batch_size = int(slice_info["num"])

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

    def init_pipeline(self) -> None:
        self.init_sahi()
        super().init_pipeline()

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.get("properties", {})
            if name == "nvsahipreprocess":
                properties["config-file"] = str(
                    config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
                )

    def write_sahi(self, config_save_dir: Path) -> None:
        self.nvsahipreprocess_generator.write(
            config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
        )
        sahi = self.sahi["nvsahipreprocess"]
        sahi_info = get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=sahi.get("enable_full_frame", True),
        )
        sahi_show = get_sahi_preview(sahi_info)
        sahi_show.save(config_save_dir / "sahi_slice_preview.jpg")
        with open(config_save_dir / "sahi_slice_info.json", "w", encoding="utf-8") as handle:
            json.dump(sahi_info, handle)

    def write(self, config_save_dir: str | Path) -> None:
        super().write(config_save_dir)
        self.write_sahi(Path(config_save_dir))

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
        postprocess = self.sahi[self.SAHI_POSTPROCESS]
        self._append_node(
            "nvsahipreprocess",
            "nvsahipreprocess",
            self._add_nvsahipreprocess(
                self.SAHI_PREPROCESS_CONFIG_NAME,
                slice_width=sahi["slice_width"],
                slice_height=sahi["slice_height"],
                overlap_width_ratio=sahi["overlap_width_ratio"],
                overlap_height_ratio=sahi["overlap_height_ratio"],
                enable_full_frame=sahi.get("enable_full_frame", True),
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
            self.SAHI_POSTPROCESS,
            self.SAHI_POSTPROCESS,
            self.sahi_postprocess_properties(postprocess),
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
        gpu_id = self.pgie_generator.gpu_id
        if self.drawer is not None:
            drawer = self.drawer
            self._append_node(
                self.nvdet_drawer_element(),
                "nvdetfadedrawer",
                self.nvdet_drawer_properties(drawer),
            )
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
        self._append_node(
            "nvdetlogger",
            "nvdetlogger",
            self._add_nvdetlogger(
                root=f"/root/logs/deepstream/{self.pipeline_name}",
                interval=int(self.logger.get("interval", 0)),
            ),
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
            "nvstreammux": "nvsahipreprocess",
            "nvsahipreprocess": "pgie",
            "pgie": "queue_sahi",
            "queue_sahi": self.SAHI_POSTPROCESS,
            self.SAHI_POSTPROCESS: "nvdsanalytics",
        }
        edges["nvdsanalytics"] = self.after_analytics()
        self.link_event_coder(edges)
        vis_next = "nvosdbin"
        if self.drawer is not None:
            vis_next = "nvdetfadedrawer"
        edges["tee_msg"] = [vis_next, "queue_msg"]
        if self.drawer is not None:
            edges["nvdetfadedrawer"] = "nvosdbin"
        edges["queue_msg"] = "nvmsgconv"
        edges["nvmsgconv"] = "nvmsgbroker"
        edges["nvosdbin"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "nvdetlogger"
        edges["nvdetlogger"] = "nvjpegenc"
        edges["nvjpegenc"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges

    def sahi_postprocess_properties(self, postprocess: dict) -> dict:
        return self._add_nvsahipostprocess(
            gie_ids=str(self.pgie_generator.config["property"]["gie-unique-id"]),
            match_metric=1,
            match_threshold=postprocess["match_threshold"],
            class_agnostic=False,
            enable_merge=postprocess.get("enable_merge", True),
            two_phase_nmm=True,
        )
