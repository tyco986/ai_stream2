from pathlib import Path

from ..base_generator.base_vis_video import BaseVisVideoGenerator
from ..subelement_generator.nvtracker import TRACKER_LL_LIB
from .stgcnpp_yolo_pose_mixin import StgcnppYoloPoseMixin

STGCNPP_YOLO_POSE_VIS_VIDEO_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdspreprocess → sgie1
            → nvdsanalytics → nvosdbin → nvvideoconvert → nvv4l2h264enc → h264parse
            → mp4mux → filesink
"""


class StgcnppYoloPoseVisVideoGenerator(StgcnppYoloPoseMixin, BaseVisVideoGenerator):
    GENERATOR = "StgcnppYoloPoseVisVideoGenerator"

    f"""Generate ST-GCN++ vis-video pipeline YAML with OSD and mp4 filesink.

    {STGCNPP_YOLO_POSE_VIS_VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        stgcnpp: dict,
        tracker: dict | None = None,
        logger: dict | None = None,
        drawer: dict | None = None,
        event_coder: dict | None = None,
    ) -> None:
        self.stgcnpp = stgcnpp
        super().__init__(
            pipeline_name=pipeline_name,
            input=input,
            output=output,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
            logger=logger,
            drawer=drawer,
            event_coder=event_coder,
        )

    def add(self) -> None:
        self._append_node(
            "nvurisrcbin",
            "nvurisrcbin",
            self._add_nvurisrcbin(
                self.file_uri(self.input),
                disable_audio=True,
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
        self.append_stgcnpp_nodes()
        self._append_node(
            "nvdsanalytics",
            "nvdsanalytics",
            self._add_nvdsanalytics(
                self.ANALYTICS_CONFIG_NAME,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self.append_event_coder()
        self.append_kafka_nodes()
        gpu_id = self.pgie_generator.gpu_id
        if self.drawer is not None:
            drawer = self.drawer
            self._append_node(
                self.nvpose_drawer_element(),
                "nvposefadedrawer",
                self.nvpose_drawer_properties(drawer),
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
        self._append_node(
            "nvv4l2h264enc",
            "nvv4l2h264enc",
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
            "filesink",
            self._add_filesink(self.output, sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "nvurisrcbin": "nvstreammux",
            "nvstreammux": "pgie",
        }
        inference_tail = "pgie"
        if self.enable_nvtracker:
            if self.drawer is not None:
                edges[inference_tail] = "nvbboxsnapshot"
                edges["nvbboxsnapshot"] = "nvtracker"
            else:
                edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        self.link_stgcnpp(edges, inference_tail)
        self.link_kafka_from_analytics(edges, self.vis_tee_next())
        self.link_drawer_before_osd(edges)
        edges["nvosdbin"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "nvdetlogger"
        edges["nvdetlogger"] = "nvv4l2h264enc"
        edges["nvv4l2h264enc"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges
