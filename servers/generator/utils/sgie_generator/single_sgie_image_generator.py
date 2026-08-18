from pathlib import Path

from ..base_generator.base_image import BaseImageGenerator
from .single_sgie_mixin import SingleSgieMixin

SINGLE_SGIE_IMAGE_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvinfer → sgie → nvdsanalytics → nvosdbin
            → nvvideoconvert → nvjpegenc → filesink
"""


class SingleSgieImageGenerator(SingleSgieMixin, BaseImageGenerator):
    GENERATOR = "SingleSgieImageGenerator"
    SINK_PATH_TEMPLATES = {
        "filesink": [
            "nvurisrcbin",
            "nvstreammux",
            "nvinfer",
            "sgie",
            "nvdsanalytics",
            "nvosdbin",
            "nvvideoconvert",
            "nvjpegenc",
            "filesink",
        ],
    }

    f"""Generate single-SGIE image pipeline YAML.

    {SINGLE_SGIE_IMAGE_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
    ) -> None:
        self.sgie = sgie
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
            "nvinfer",
            self._add_nvinfer(
                config_file_path=self.PGIE_CONFIG_NAME,
                batch_size=self.pgie_generator.batch_size,
                gpu_id=self.pgie_generator.gpu_id,
            ),
        )
        self.append_sgie_node()
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
            "nvstreammux": "nvinfer",
            "nvinfer": "sgie",
            "sgie": "nvdsanalytics",
            "nvdsanalytics": "nvosdbin",
            "nvosdbin": "nvvideoconvert",
            "nvvideoconvert": "nvjpegenc",
            "nvjpegenc": "filesink",
        }
        self.pipeline["deepstream"]["edges"] = edges
