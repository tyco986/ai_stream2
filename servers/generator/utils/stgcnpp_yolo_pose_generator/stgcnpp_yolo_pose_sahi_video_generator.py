from pathlib import Path

from ..base_generator.base_sahi_pose_video import BaseSahiPoseVideoGenerator
from ..subelement_generator.nvtracker import TRACKER_LL_LIB
from .stgcnpp_yolo_pose_mixin import StgcnppYoloPoseMixin

STGCNPP_YOLO_POSE_SAHI_VIDEO_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
            → nvbboxsnapshot → nvtracker → nvdspreprocess → sgie1 → nvdsanalytics
            → nvvideoconvert → fakesink

    Notes::

        ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
"""


class StgcnppYoloPoseSahiVideoGenerator(StgcnppYoloPoseMixin, BaseSahiPoseVideoGenerator):
    GENERATOR = "StgcnppYoloPoseSahiVideoGenerator"

    f"""Generate ST-GCN++ SAHI video pipeline YAML (headless, ends at fakesink).

    {STGCNPP_YOLO_POSE_SAHI_VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        pipeline_name: str,
        input: str | Path,
        analyzer: dict | None,
        pgie: dict,
        stgcnpp: dict,
        sahi: dict,
        tracker: dict | None = None,
        logger: dict | None = None,
        drawer: dict | None = None,
        event_coder: dict | None = None,
    ) -> None:
        self.stgcnpp = stgcnpp
        super().__init__(
            pipeline_name=pipeline_name,
            input=input,
            analyzer=analyzer,
            pgie=pgie,
            sahi=sahi,
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
        self._append_node(
            "nvdetlogger",
            "nvdetlogger",
            self._add_nvdetlogger(
                root=f"/root/logs/deepstream/{self.pipeline_name}",
                interval=int(self.logger.get("interval", 0)),
            ),
        )
        self._append_node(
            "fakesink",
            "fakesink",
            self._add_fakesink(sync=False, async_=False),
        )

    def link(self) -> None:
        edges = {
            "nvurisrcbin": "nvstreammux",
            "nvstreammux": "nvsahipreprocess",
            "nvsahipreprocess": "pgie",
            "pgie": "queue_sahi",
            "queue_sahi": self.SAHI_POSTPROCESS,
        }
        inference_tail = self.SAHI_POSTPROCESS
        if self.enable_nvtracker:
            if self.drawer is not None:
                edges[inference_tail] = "nvbboxsnapshot"
                edges["nvbboxsnapshot"] = "nvtracker"
            else:
                edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        self.link_stgcnpp(edges, inference_tail)
        self.link_kafka_from_analytics(edges, "nvdetlogger")
        edges["nvdetlogger"] = "fakesink"
        self.pipeline["deepstream"]["edges"] = edges
