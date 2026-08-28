import copy

from ..subelement_generator.kafka import KAFKA_CONN_STR, KAFKA_PROTO_LIB
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL
from ..subelement_generator.utils.default_gie import YoloPose
from ..stgcnpp_rtmpose_generator.stgcnpp_core_mixin import StgcnppCoreMixin


class StgcnppYoloPoseMixin(StgcnppCoreMixin):
    def init_pipeline(self) -> None:
        self.init_stgcnpp()
        super().init_pipeline()

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }

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
