from pathlib import Path

from ..base_generator.base_image import BaseImageGenerator
from .stgcnpp_mixin import StgcnppMixin

STGCNPP_IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → sgie0 → nvdspreprocess → sgie1
            → nvdsanalytics → nvosdbin → nvvideoconvert → nvjpegenc → filesink
"""


class StgcnppImageGenerator(StgcnppMixin, BaseImageGenerator):
    GENERATOR = "StgcnppImageGenerator"
    SINK_PATH_TEMPLATES = {
        "filesink": [
            "nvurisrcbin",
            "nvstreammux",
            "pgie",
            "sgie0",
            "nvdspreprocess",
            "sgie1",
            "nvdsanalytics",
            "nvosdbin",
            "nvvideoconvert",
            "nvjpegenc",
            "filesink",
        ],
    }

    f"""Generate ST-GCN++ image pipeline YAML.

    {STGCNPP_IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
        stgcnpp: dict,
    ) -> None:
        self.sgie = sgie
        self.stgcnpp = stgcnpp
        super().__init__(
            input=input,
            output=output,
            analyzer=analyzer,
            pgie=pgie,
        )

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
            "nvosdbin",
            "nvosdbin",
            self._add_nvosdbin(**self.osd_kwargs(gpu_id)),
        )
        self._append_node(
            "nvvideoconvert",
            "nvvideoconvert",
            self._add_nvvideoconvert(gpu_id=gpu_id),
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
            "nvstreammux": "pgie",
            "pgie": "sgie0",
        }
        self.link_stgcnpp(edges)
        edges["nvdsanalytics"] = "nvosdbin"
        edges["nvosdbin"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "nvjpegenc"
        edges["nvjpegenc"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges
