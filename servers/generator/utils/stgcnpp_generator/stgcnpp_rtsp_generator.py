from ..base_generator.base_rtsp import BaseRTSPGenerator
from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .stgcnpp_mixin import StgcnppMixin

STGCNPP_RTSP_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin{N} → nvstreammux → pgie → nvtracker → sgie0 → nvdspreprocess
            → sgie1 → nvdsanalytics → nvstreamdemux
            → queue_demux{N} → nvvideoconvert{N} → fakesink{N}
"""


class StgcnppRTSPGenerator(StgcnppMixin, BaseRTSPGenerator):
    GENERATOR = "StgcnppRTSPGenerator"
    SINK_PATH_TEMPLATES = {
        "fakesink{index}": [
            "nvurisrcbin{index}",
            "nvstreammux",
            "pgie",
            "nvtracker",
            "sgie0",
            "nvdspreprocess",
            "sgie1",
            "nvdsanalytics",
            "nvstreamdemux",
            "queue_demux{index}",
            "nvvideoconvert{index}",
            "fakesink{index}",
        ],
    }

    f"""Generate ST-GCN++ RTSP pipeline (headless).

    {STGCNPP_RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
        stgcnpp: dict,
        tracker: dict | None = None,
    ) -> None:
        self.sgie = sgie
        self.stgcnpp = stgcnpp
        super().__init__(
            streams=streams,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
        )

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
        self._append_node("nvstreamdemux", "nvstreamdemux", self._add_nvstreamdemux())
        gpu_id = self.pgie_generator.gpu_id
        for index in range(len(self.streams)):
            self._append_node("queue", f"queue_demux{index}", self._add_queue())
            self._append_node(
                "nvvideoconvert",
                f"nvvideoconvert{index}",
                self._add_nvvideoconvert(gpu_id=gpu_id),
            )
            self._append_node(
                "fakesink",
                f"fakesink{index}",
                self._add_fakesink(sync=False, async_=False),
            )

    def link(self) -> None:
        self.pad_links = {"nvstreamdemux": []}
        edges: dict = {}
        for index in range(len(self.streams)):
            edges[f"nvurisrcbin{index}"] = "nvstreammux"
        edges["nvstreammux"] = "pgie"
        inference_tail = "pgie"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "sgie0"
        self.link_stgcnpp(edges)
        edges["nvdsanalytics"] = "nvstreamdemux"
        for index in range(len(self.streams)):
            self.pad_links["nvstreamdemux"].append(f"queue_demux{index}")
            edges[f"queue_demux{index}"] = f"nvvideoconvert{index}"
            edges[f"nvvideoconvert{index}"] = f"fakesink{index}"
        self.pipeline["deepstream"]["edges"] = edges
