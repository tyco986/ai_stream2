from pathlib import Path

from ..base_generator.base_sahi_video import BaseSahiVideoGenerator
from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .stgcnpp_mixin import StgcnppMixin

STGCNPP_SAHI_VIDEO_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
            → nvtracker → sgie0 → nvdspreprocess → sgie1 → nvdsanalytics
            → nvvideoconvert → fakesink

    Notes::

        ``mux`` batch size is 1; ``pgie`` batch size is the SAHI tile count.
"""


class StgcnppSahiVideoGenerator(StgcnppMixin, BaseSahiVideoGenerator):
    GENERATOR = "StgcnppSahiVideoGenerator"
    SINK_PATH_TEMPLATES = {
        "fakesink": [
            "nvurisrcbin",
            "nvstreammux",
            "nvsahipreprocess",
            "pgie",
            "queue_sahi",
            "nvsahipostprocess",
            "nvtracker",
            "sgie0",
            "nvdspreprocess",
            "sgie1",
            "nvdsanalytics",
            "nvvideoconvert",
            "fakesink",
        ],
    }

    f"""Generate ST-GCN++ SAHI video pipeline YAML (headless, ends at fakesink).

    {STGCNPP_SAHI_VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
        stgcnpp: dict,
        sahi: dict,
        tracker: dict | None = None,
    ) -> None:
        self.sgie = sgie
        self.stgcnpp = stgcnpp
        super().__init__(
            input=input,
            analyzer=analyzer,
            pgie=pgie,
            sahi=sahi,
            tracker=tracker,
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
        postprocess = self.sahi["nvsahipostprocess"]
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
            "nvsahipostprocess",
            "nvsahipostprocess",
            self._add_nvsahipostprocess(
                gie_ids=str(self.pgie_generator.config["property"]["gie-unique-id"]),
                match_metric=1,
                match_threshold=postprocess["match_threshold"],
                class_agnostic=False,
                enable_merge=True,
                two_phase_nmm=True,
            ),
        )
        if self.enable_nvtracker:
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
        self.append_sgie_node()
        self.append_stgcnpp_nodes()
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
            "nvvideoconvert",
            "nvvideoconvert",
            self._add_nvvideoconvert(gpu_id=gpu_id),
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
            "queue_sahi": "nvsahipostprocess",
        }
        inference_tail = "nvsahipostprocess"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "sgie0"
        self.link_stgcnpp(edges)
        edges["nvdsanalytics"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "fakesink"
        self.pipeline["deepstream"]["edges"] = edges
