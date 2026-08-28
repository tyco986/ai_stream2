import copy

from ..base_generator.base_rtsp_vis import BaseRTSPVisGenerator, VIS_RTSP_TOPOLOGY_DOC
from ..subelement_generator.kafka import KAFKA_CONN_STR, KAFKA_PROTO_LIB
from ..subelement_generator.nvmsgconv import PAYLOAD_DEEPSTREAM_MINIMAL
from ..subelement_generator.nvtracker import TRACKER_LL_LIB
from ..subelement_generator.utils.default_gie import YoloSeg


class SegVisRTSPGenerator(BaseRTSPVisGenerator):
    GENERATOR = "SegVisRTSPGenerator"

    f"""Generate YOLO segmentation RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
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
            "pgie",
            self._add_nvinfer(
                config_file_path=self.PGIE_CONFIG_NAME,
                batch_size=self.pgie_generator.batch_size,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        if self.enable_nvtracker:
            if self.drawer is not None:
                self._append_node(
                    "nvbboxsnapshot",
                    "nvbboxsnapshot",
                    self._add_nvbboxsnapshot(),
                )
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
        self._append_node("nvstreamdemux", "nvstreamdemux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        osd_kwargs = self.event_osd_kwargs(gpu_id)
        for index, name in enumerate(self.streams):
            sink_uri = self.visualized_sink_uri(self.streams[name]["url"])
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            if self.drawer is not None:
                drawer = self.drawer
                self._append_node(
                    self.nvseg_drawer_element(),
                    f"nvsegfadedrawer{index}",
                    self.nvseg_drawer_properties(drawer),
                )
            self._append_node(
                "nvosdbin",
                f"nvosdbin{index}",
                self._add_nvosdbin(**osd_kwargs),
            )
            self._append_node("queue", f"queue_enc{index}", self._add_queue())
            self._append_node(
                "nvdetlogger",
                f"nvdetlogger{index}",
                self._add_nvdetlogger(
                    root=f"/root/logs/deepstream/{self.pipeline_name}",
                    interval=int(self.logger.get("interval", 0)),
                ),
            )
            self._append_node(
                "nvv4l2h264enc",
                f"nvv4l2h264enc{index}",
                self._add_nvv4l2h264enc(
                    bitrate=4_000_000,
                    iframeinterval=self.fps,
                    preset_id=1,
                    gpu_id=gpu_id,
                ),
            )
            self._append_node("h264parse", f"h264parse{index}", self._add_h264parse())
            self._append_node(
                "rtspclientsink",
                f"rtspclientsink{index}",
                self._add_rtspclientsink(location=sink_uri, sync=False, async_=False),
            )

    def link(self) -> None:
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "pgie"
        inference_tail = "pgie"
        if self.enable_nvtracker:
            if self.drawer is not None:
                edges[inference_tail] = "nvbboxsnapshot"
                edges["nvbboxsnapshot"] = "nvtracker"
            else:
                edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = self.after_analytics()
        self.link_event_coder(edges)
        edges["tee_msg"] = ["nvstreamdemux", "queue_msg"]
        edges["queue_msg"] = "nvmsgconv"
        edges["nvmsgconv"] = "nvmsgbroker"
        for index in range(len(self.streams)):
            edges[f"queue_demux{index}"] = f"nvvideoconvert{index}"
            osd_prev = f"nvosdbin{index}"
            if self.drawer is not None:
                osd_prev = f"nvsegfadedrawer{index}"
            edges[f"nvvideoconvert{index}"] = osd_prev
            if self.drawer is not None:
                edges[f"nvsegfadedrawer{index}"] = f"nvosdbin{index}"
            edges[f"nvosdbin{index}"] = f"queue_enc{index}"
            edges[f"queue_enc{index}"] = f"nvdetlogger{index}"
            edges[f"nvdetlogger{index}"] = f"nvv4l2h264enc{index}"
            edges[f"nvv4l2h264enc{index}"] = f"h264parse{index}"
            edges[f"h264parse{index}"] = f"rtspclientsink{index}"
        self.pipeline["deepstream"]["edges"] = edges
