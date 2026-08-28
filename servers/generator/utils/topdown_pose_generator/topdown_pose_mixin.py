from pathlib import Path

import yaml

from ..subelement_generator.kafka import KAFKA_CONN_STR, KAFKA_PROTO_LIB
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL
from ..subelement_generator.rtmpose_preprocess import NvdspreprocessRtmposeGenerator
from ..subelement_generator.sgie import SgieGenerator
from ..subelement_generator.utils.default_gie.manager import PgieManager, SgieManager
from ..subelement_generator.utils.sgie_parser import SgieParser


class TopdownPoseMixin:
    SGIE_CONFIG_NAME = "sgie0.yml"
    RTMPOSE_PREPROCESS_CONFIG_NAME = "nvdspreprocess_rtmpose.txt"

    def init_pipeline(self) -> None:
        self.init_sgie()
        self.init_nvdspreprocess_rtmpose()
        super().init_pipeline()

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["sgie"] = self.sgie

    def stream_count(self) -> int:
        count = 1
        streams = getattr(self, "streams", None)
        if streams is not None:
            count = len(streams)
        return count

    def init_sgie(self) -> None:
        self.sgie = {
            "model_dir": self.sgie["model_dir"],
            "interval": int(self.sgie.get("interval", 1)),
        }
        self.sgie_config_parser = SgieParser(
            self.sgie["model_dir"],
            self.sgie["interval"],
        )
        self.sgie_generator = SgieGenerator(**self.sgie_config_parser.build())
        self.apply_sgie_config()
        self.params_yml["sgie"] = self.sgie

    def init_nvdspreprocess_rtmpose(self) -> None:
        input_shape = self.sgie_config_parser.meta["input_tensor_shape"]
        input_names = self.sgie_config_parser.meta.get("input_tensor_names") or [None]
        self.rtmpose_preprocess_generator = NvdspreprocessRtmposeGenerator(
            batch_size=self.sgie_generator.batch_size,
            infer_width=int(input_shape[3]),
            infer_height=int(input_shape[2]),
            channels=int(input_shape[1]),
            stream_count=self.stream_count(),
            tensor_name=input_names[0],
        )
        self.rtmpose_preprocess_ini = self.rtmpose_preprocess_generator.render()

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = PgieManager().config(
            self.pgie_config_parser.meta["version"]
        )
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def apply_sgie_config(self) -> None:
        self.sgie_generator.config = SgieManager().config(
            self.sgie_config_parser.meta["version"]
        )
        self.sgie_generator.update_config()
        self.sgie_yml = self.sgie_generator.config

    def append_sgie_node(self) -> None:
        self._append_node(
            "nvdspreprocess",
            "nvdspreprocess_rtmpose",
            self._add_nvdspreprocess(self.RTMPOSE_PREPROCESS_CONFIG_NAME),
        )
        self._append_node(
            "nvinfer",
            "sgie0",
            self._add_nvinfer(
                config_file_path=self.SGIE_CONFIG_NAME,
                batch_size=self.sgie_generator.batch_size,
                gpu_id=self.sgie_generator.gpu_id,
                input_tensor_meta=True,
            ),
        )
        self._append_node(
            "nvrtmposepostprocess",
            "nvrtmposepostprocess",
            self.rtmpose_postprocess_properties(),
        )

    def rtmpose_postprocess_properties(self) -> dict:
        input_shape = self.sgie_config_parser.meta["input_tensor_shape"]
        sgie_id = int(self.sgie_yml["property"].get("gie-unique-id", 2))
        return self._add_nvrtmposepostprocess(
            infer_width=int(input_shape[3]),
            infer_height=int(input_shape[2]),
            padding=float(NvdspreprocessRtmposeGenerator.PADDING),
            sgie_unique_id=sgie_id,
        )

    def pose_gie_tail(self) -> str:
        return "nvrtmposepostprocess"

    def link_sgie_from(self, edges: dict, src: str) -> None:
        edges[src] = "nvdspreprocess_rtmpose"
        edges["nvdspreprocess_rtmpose"] = "sgie0"
        edges["sgie0"] = "nvrtmposepostprocess"

    def vis_tee_next(self) -> str:
        name = "nvosdbin"
        if self.drawer is not None:
            name = "nvposefadedrawer"
        return name

    def link_drawer_before_osd(self, edges: dict) -> None:
        if self.drawer is not None:
            edges["nvposefadedrawer"] = "nvosdbin"

    def append_kafka_nodes(self) -> None:
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

    def link_kafka_from_analytics(self, edges: dict, video_next: str) -> None:
        edges["nvdsanalytics"] = self.after_analytics()
        self.link_event_coder(edges)
        edges["tee_msg"] = [video_next, "queue_msg"]
        edges["queue_msg"] = "nvmsgconv"
        edges["nvmsgconv"] = "nvmsgbroker"

    def apply_save_paths(self, config_save_dir: Path) -> None:
        super().apply_save_paths(config_save_dir)
        for node in self.pipeline_yml["deepstream"]["nodes"]:
            name = node["name"]
            properties = node.setdefault("properties", {})
            if name == "sgie0":
                properties["config-file-path"] = str(
                    config_save_dir / self.SGIE_CONFIG_NAME
                )
            if name == "nvdspreprocess_rtmpose":
                properties["config-file"] = str(
                    config_save_dir / self.RTMPOSE_PREPROCESS_CONFIG_NAME
                )

    def write(self, config_save_dir: str | Path) -> None:
        super().write(config_save_dir)
        config_save_dir = Path(config_save_dir)
        with open(config_save_dir / self.SGIE_CONFIG_NAME, "w", encoding="utf-8") as handle:
            yaml.safe_dump(
                self.sgie_yml, handle, sort_keys=False, default_flow_style=False
            )
        (
            config_save_dir / self.RTMPOSE_PREPROCESS_CONFIG_NAME
        ).write_text(self.rtmpose_preprocess_ini, encoding="utf-8")
