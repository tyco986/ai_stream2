import copy
import json
import shutil
from pathlib import Path

import yaml
from PIL import Image

from .base import (
    DeepstreamConfigGenerator,
    KAFKA_CONN_STR,
    KAFKA_PROTO_LIB,
    KAFKA_TOPIC,
    TRACKER_LL_LIB,
    align_tracker_height,
)
from ..subelement_generator import (
    KafkaConfigGenerator,
    NvdsanalyticsConfigGenerator,
    NvmsgconvConfigGenerator,
    NvtrackerConfigGenerator,
    NvsahipreprocessConfigGenerator,
    PgieConfigGenerator,
)
from .utils import YoloDet, YoloDetSahi, YoloPose, YoloSeg, get_sahi_box, get_sahi_preview
from .utils.pgie_config_parser import PgieConfigParser
from .utils.validate_video import probe_video


class YoloDetVideoConfigGenerator(DeepstreamConfigGenerator):
    GENERATOR = "YoloDetVideoConfigGenerator"

    PIPELINE_CONFIG_NAME = "pipeline.yml"
    MSGCONV_CONFIG_NAME = "nvmsgconv.yml"
    PGIE_CONFIG_NAME = "pgie.yml"
    KAFKA_CONFIG_NAME = "kafka.txt"
    TRACKER_CONFIG_NAME = "nvtracker.yml"
    ANALYTICS_CONFIG_NAME = "nvdsanalytics.yml"
    PARAMS_NAME = "params.yml"

    """Generate YOLO detection video pipeline YAML.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``.

    Parameters:
        input: source video path
        output: destination video path for annotated output
        config_save_dir: directory for generated configs (pipeline.yml, etc.)
        enable_kafka: tee branch with nvmsgconv → nvmsgbroker
        enable_nvtracker: generate ``nvtracker.yml`` and insert nvtracker; default False
        enable_nvdsanalytics: generate ``nvdsanalytics.yml`` and insert nvdsanalytics; default False
        pgie_model_dir: model directory containing meta.json, labels.txt, and .engine

    Topology (``enable_kafka``):

    1. enable_kafka=False — inference + annotated video file::

        src → mux → pgie → [nvtracker] → [nvdsanalytics]
              → osd → nvvidconv → encoder → h264parse → mp4mux → filesink

    2. enable_kafka=True — inference + Kafka metadata + annotated video file::

        src → mux → pgie → [nvtracker] → [nvdsanalytics] → tee
              ─┬→ queue_meta → msgconv → msgbroker
               └→ osd → nvvidconv → encoder → h264parse → mp4mux → filesink
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        pgie_model_dir: str | Path,
        config_save_dir: str | Path,
        enable_kafka: bool,
        enable_nvtracker: bool = False,
        interval: int = 0,
        pgie_class_attr={"all": {"conf": 0.25}},
        pgie_class_on: list[int] | None = None,
        nvdsanalytics_config: dict | None = None,
    ) -> None:
        super().__init__()

        if pgie_class_on is not None:
            assert len(pgie_class_on) == len(set(pgie_class_on)), (
                "pgie_class_on contains duplicate class ids"
            )
            pgie_class_on = list(set(pgie_class_on))

        self.input = Path(input).expanduser().resolve()
        self.output = Path(output).expanduser().resolve()
        self.pgie_model_dir = pgie_model_dir
        self.pgie_class_attr = pgie_class_attr
        self.pgie_class_on = pgie_class_on
        self.config_save_dir = Path(config_save_dir)
        self.enable_kafka = enable_kafka
        self.enable_nvtracker = enable_nvtracker
        self.nvdsanalytics_config = nvdsanalytics_config
        self.enable_nvdsanalytics = nvdsanalytics_config is not None
        self.interval = interval

        video_info = probe_video(self.input)
        self.width = video_info["width"]
        self.height = video_info["height"]
        self.fps = video_info["fps"]
        self.runtime_batch_size = self.resolve_pgie_runtime_batch_size()

        self.init_pgie_generator()

        if self.enable_kafka:
            self.kafka_generator = KafkaConfigGenerator()
            self.nvmsgconv_generator = NvmsgconvConfigGenerator([self.file_uri(self.input)])

        self.output.parent.mkdir(parents=True, exist_ok=True)
        self.pipeline_save_path = self.config_save_dir / self.PIPELINE_CONFIG_NAME
        self.pgie_save_path = self.config_save_dir / self.PGIE_CONFIG_NAME
        self.msgconv_save_path = self.config_save_dir / self.MSGCONV_CONFIG_NAME
        self.kafka_save_path = self.config_save_dir / self.KAFKA_CONFIG_NAME
        self.nvtracker_save_path = self.config_save_dir / self.TRACKER_CONFIG_NAME
        self.nvdsanalytics_save_path = self.config_save_dir / self.ANALYTICS_CONFIG_NAME
        self.params_save_path = self.config_save_dir / self.PARAMS_NAME

        if self.enable_nvtracker:
            self.nvtracker_generator = NvtrackerConfigGenerator()
        if self.enable_nvdsanalytics:
            self.nvdsanalytics_generator = NvdsanalyticsConfigGenerator(
                self.nvdsanalytics_config
            )

        self.before_build_pipeline()
        self.add()
        self.link()
        self.in_params = self.get_in_params()

    def get_in_params(self) -> dict:
        return {
            "generator": self.GENERATOR,
            "input": str(self.input),
            "output": str(self.output),
            "pgie_model_dir": str(self.pgie_model_dir),
            "config_save_dir": str(self.config_save_dir),
            "enable_kafka": self.enable_kafka,
            "enable_nvtracker": self.enable_nvtracker,
            "interval": self.interval,
            "pgie_class_attr": self.pgie_class_attr,
            "pgie_class_on": self.pgie_class_on,
            "nvdsanalytics_config": self.nvdsanalytics_config,
        }

    def resolve_pgie_runtime_batch_size(self) -> int:
        return 1

    def before_build_pipeline(self) -> None:
        pass

    def pgie_task_template(self) -> dict:
        return copy.deepcopy(YoloDet)

    def init_pgie_generator(self) -> None:
        self.pgie_config_parser = PgieConfigParser(
            self.pgie_model_dir,
            self.runtime_batch_size,
            self.pgie_class_attr,
            self.pgie_class_on,
            self.interval,
        )
        self.pgie_generator = PgieConfigGenerator(**self.pgie_config_parser.build())
        self.pgie_generator.config = self.pgie_task_template()
        self.pgie_generator.update_config()

    def write(self) -> None:
        shutil.copy2(
            self.pgie_config_parser.meta_path,
            self.config_save_dir / self.pgie_config_parser.meta_path.name,
        )
        self.pgie_generator.write(self.pgie_save_path)
        if self.enable_kafka:
            self.kafka_generator.write(self.kafka_save_path)
            self.nvmsgconv_generator.write(self.msgconv_save_path)
        if self.enable_nvtracker:
            self.nvtracker_generator.write(self.nvtracker_save_path)
        if self.enable_nvdsanalytics:
            self.nvdsanalytics_generator.write(self.nvdsanalytics_save_path)
        with open(self.pipeline_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self._config, handle, sort_keys=False, default_flow_style=False)
        with open(self.params_save_path, "w", encoding="utf-8") as handle:
            yaml.safe_dump(self.in_params, handle, sort_keys=False, default_flow_style=False)

    def add(self) -> None:
        self.add_source_and_mux()
        self.add_pgie_node()
        self.add_kafka_nodes()
        self.add_tracker_and_analytics_nodes()
        self.add_sink_chain()

    def add_source_and_mux(self) -> None:
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

    def add_pgie_node(self) -> None:
        self._append_node(
            "nvinfer",
            "pgie",
            self._add_nvinfer(
                config_file_path=str(self.pgie_save_path),
                batch_size=self.pgie_generator.batch_size,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )

    def add_sink_chain(self) -> None:
        self._append_node(
            "nvosdbin",
            "osd",
            self._add_nvosdbin(
                gpu_id=self.pgie_generator.gpu_id,
                display_bbox=True,
                display_text=True,
            ),
        )
        self._append_node(
            "nvvideoconvert",
            "nvvidconv",
            self._add_nvvideoconvert(gpu_id=self.pgie_generator.gpu_id),
        )
        self._append_node(
            "nvv4l2h264enc",
            "encoder",
            self._add_nvv4l2h264enc(
                bitrate=4_000_000,
                iframeinterval=self.fps,
                preset_id=1,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self._append_node(
            "h264parse",
            "h264parse",
            self._add_h264parse(),
        )
        self._append_node(
            "mp4mux",
            "mp4mux",
            self._add_mp4mux(),
        )
        self._append_node(
            "filesink",
            "sink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def add_kafka_nodes(self) -> None:
        if not self.enable_kafka:
            return

        self._append_node("tee", "tee", self._add_tee())
        self._append_node("queue", "queue_meta", self._add_queue())
        self._append_node(
            "nvmsgconv",
            "msgconv",
            self._add_nvmsgconv(
                str(self.msgconv_save_path),
                payload_type=1,
                msg2p_newapi=True,
                frame_interval=self.pgie_generator.interval + 1,
            ),
        )
        self._append_node(
            "nvmsgbroker",
            "msgbroker",
            self._add_nvmsgbroker(
                proto_lib=KAFKA_PROTO_LIB,
                conn_str=KAFKA_CONN_STR,
                topic=KAFKA_TOPIC,
                broker_config=str(self.kafka_save_path),
            ),
        )

    def add_tracker_and_analytics_nodes(self) -> None:
        if self.enable_nvtracker:
            self._append_node(
                "nvtracker",
                "tracker",
                self._add_nvtracker(
                    TRACKER_LL_LIB,
                    str(self.nvtracker_save_path),
                    tracker_width=self.width,
                    tracker_height=align_tracker_height(self.height),
                    gpu_id=self.pgie_generator.gpu_id,
                ),
            )
        if self.enable_nvdsanalytics:
            self._append_node(
                "nvdsanalytics",
                "analytics",
                self._add_nvdsanalytics(
                    str(self.nvdsanalytics_save_path),
                    gpu_id=self.pgie_generator.gpu_id,
                ),
            )

    def link_inference_tail(self, edges: dict, inference_tail: str) -> str:
        if self.enable_nvtracker:
            edges[inference_tail] = "tracker"
            inference_tail = "tracker"
        if self.enable_nvdsanalytics:
            edges[inference_tail] = "analytics"
            inference_tail = "analytics"

        if self.enable_kafka:
            edges[inference_tail] = "tee"
            edges["queue_meta"] = "msgconv"
            edges["msgconv"] = "msgbroker"
            edges["tee"] = ["queue_meta", "osd"]
            return inference_tail

        edges[inference_tail] = "osd"
        return inference_tail

    def link_sink_chain(self, edges: dict) -> None:
        edges["osd"] = "nvvidconv"
        edges["nvvidconv"] = "encoder"
        edges["encoder"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "sink"

    def link(self) -> None:
        edges: dict = {"src": "mux", "mux": "pgie"}
        self.link_inference_tail(edges, "pgie")
        self.link_sink_chain(edges)
        self._config["deepstream"]["edges"] = edges

    @staticmethod
    def file_uri(path: str | Path) -> str:
        return Path(path).expanduser().resolve().as_uri()


class YoloSegVideoConfigGenerator(YoloDetVideoConfigGenerator):
    GENERATOR = "YoloSegVideoConfigGenerator"

    def pgie_task_template(self) -> dict:
        return copy.deepcopy(YoloSeg)


class YoloPoseVideoConfigGenerator(YoloDetVideoConfigGenerator):
    GENERATOR = "YoloPoseVideoConfigGenerator"

    def pgie_task_template(self) -> dict:
        return copy.deepcopy(YoloPose)


class YoloDetSahiVideoConfigGenerator(YoloDetVideoConfigGenerator):
    GENERATOR = "YoloDetSahiVideoConfigGenerator"

    """Generate YOLO SAHI detection video pipeline YAML.

    Pipeline inference chain::

        mux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → [nvtracker] → [nvdsanalytics] → [tee] → osd → filesink

    ``mux_batch_size`` is 1; ``runtime_batch_size`` is the SAHI tile count.
    """

    SAHI_PREPROCESS_CONFIG_NAME = "nvsahipreprocess.ini"

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        pgie_model_dir: str | Path,
        config_save_dir: str | Path,
        enable_kafka: bool,
        sahi_config: dict,
        enable_nvtracker: bool = False,
        interval: int = 0,
        pgie_class_attr={"all": {"conf": 0.25}},
        pgie_class_on: list[int] | None = None,
        nvdsanalytics_config: dict | None = None,
    ) -> None:
        self.sahi_config = sahi_config
        super().__init__(
            input=input,
            output=output,
            pgie_model_dir=pgie_model_dir,
            config_save_dir=config_save_dir,
            enable_kafka=enable_kafka,
            enable_nvtracker=enable_nvtracker,
            interval=interval,
            pgie_class_attr=pgie_class_attr,
            pgie_class_on=pgie_class_on,
            nvdsanalytics_config=nvdsanalytics_config,
        )

    def get_in_params(self) -> dict:
        params = super().get_in_params()
        params["sahi_config"] = self.sahi_config
        return params

    def pgie_task_template(self) -> dict:
        return copy.deepcopy(YoloDetSahi)

    def resolve_pgie_runtime_batch_size(self) -> int:
        return int(self.get_slice_boxes()["num"])

    def before_build_pipeline(self) -> None:
        meta = self.pgie_config_parser.meta
        self.nvsahipreprocess_save_path = (
            self.config_save_dir / self.SAHI_PREPROCESS_CONFIG_NAME
        )
        self.nvsahipreprocess_generator = NvsahipreprocessConfigGenerator(
            network_input_shape=self.build_network_input_shape(),
            target_unique_ids=self.pgie_generator.config["property"]["gie-unique-id"],
            tensor_data_type=0,
            tensor_name=meta["input_tensor_name"],
        )

    def build_network_input_shape(self) -> str:
        input_shape = self.pgie_config_parser.meta["input_tensor_shape"]
        return ";".join(str(value) for value in [self.runtime_batch_size, *input_shape[1:]])

    def get_slice_boxes(self) -> dict[str, int | dict[str, tuple[int, int, int, int]]]:
        sahi = self.sahi_config["nvsahipreprocess"]
        return get_sahi_box(
            image_width=self.width,
            image_height=self.height,
            slice_width=sahi["slice_width"],
            slice_height=sahi["slice_height"],
            overlap_width_ratio=sahi["overlap_width_ratio"],
            overlap_height_ratio=sahi["overlap_height_ratio"],
            enable_full_frame=True,
        )

    def preview_slice(
        self,
    ) -> tuple[dict[str, int | dict[str, tuple[int, int, int, int]]], Image.Image]:
        info = self.get_slice_boxes()
        show = get_sahi_preview(info)
        return info, show

    def add_pgie_node(self) -> None:
        sahi = self.sahi_config["nvsahipreprocess"]
        postprocess = self.sahi_config["nvsahipostprocess"]
        self._append_node(
            "nvsahipreprocess",
            "sahi_preprocess",
            self._add_nvsahipreprocess(
                str(self.nvsahipreprocess_save_path),
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
                config_file_path=str(self.pgie_save_path),
                batch_size=self.runtime_batch_size,
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

    def link(self) -> None:
        edges: dict = {"src": "mux", "mux": "sahi_preprocess"}
        edges["sahi_preprocess"] = "pgie"
        edges["pgie"] = "queue_sahi"
        edges["queue_sahi"] = "sahi_postprocess"
        self.link_inference_tail(edges, "sahi_postprocess")
        self.link_sink_chain(edges)
        self._config["deepstream"]["edges"] = edges

    def write(self) -> None:
        self.nvsahipreprocess_generator.write(self.nvsahipreprocess_save_path)
        super().write()
        sahi_info, sahi_show = self.preview_slice()
        sahi_show.save(self.config_save_dir / "sahi_slice_preview.jpg")
        with open(self.config_save_dir / "sahi_slice_info.json", "w", encoding="utf-8") as handle:
            json.dump(sahi_info, handle)
