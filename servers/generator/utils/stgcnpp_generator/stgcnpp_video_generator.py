from pathlib import Path

from ..base_generator.base_video import BaseVideoGenerator
from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .stgcnpp_mixin import StgcnppMixin

STGCNPP_VIDEO_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → nvtracker → sgie0 → nvdspreprocess → sgie1
            → nvdsanalytics → nvvideoconvert → fakesink
"""


class StgcnppVideoGenerator(StgcnppMixin, BaseVideoGenerator):
    GENERATOR = "StgcnppVideoGenerator"
    SINK_PATH_TEMPLATES = {
        "fakesink": [
            "nvurisrcbin",
            "nvstreammux",
            "pgie",
            "nvtracker",
            "sgie0",
            "nvdspreprocess",
            "sgie1",
            "nvdsanalytics",
            "nvvideoconvert",
            "fakesink",
        ],
    }

    f"""Generate ST-GCN++ video pipeline YAML (headless, ends at fakesink).

    {STGCNPP_VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
        stgcnpp: dict,
        tracker: dict | None = None,
    ) -> None:
        self.sgie = sgie
        self.stgcnpp = stgcnpp
        super().__init__(
            input=input,
            analyzer=analyzer,
            pgie=pgie,
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
            "nvstreammux": "pgie",
        }
        inference_tail = "pgie"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "sgie0"
        self.link_stgcnpp(edges)
        edges["nvdsanalytics"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "fakesink"
        self.pipeline["deepstream"]["edges"] = edges
