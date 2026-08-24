from pathlib import Path

from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .base_video import BaseVideoGenerator

VIS_VIDEO_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → pgie → nvtracker → nvdsanalytics → nvosdbin → nvvideoconvert
            → nvv4l2h264enc → h264parse → mp4mux → filesink
"""


class BaseVisVideoGenerator(BaseVideoGenerator):
    SINK_PATH_TEMPLATES = {
        "filesink": [
            "nvurisrcbin",
            "nvstreammux",
            "pgie",
            "nvtracker",
            "nvdsanalytics",
            "nvosdbin",
            "nvvideoconvert",
            "nvv4l2h264enc",
            "h264parse",
            "mp4mux",
            "filesink",
        ],
    }

    f"""Generate YOLO video pipeline YAML with OSD and mp4 filesink.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIS_VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        tracker: dict | None = None,
    ) -> None:
        self.output = Path(output).expanduser().resolve()
        super().__init__(
            input=input,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
        )

    def init_input(self) -> None:
        super().init_input()
        self.output.parent.mkdir(parents=True, exist_ok=True)

    def init_params(self) -> None:
        super().init_params()
        self.params_yml["output"] = str(self.output)

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
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "nvdsanalytics"
        edges["nvdsanalytics"] = "nvosdbin"
        edges["nvosdbin"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "nvv4l2h264enc"
        edges["nvv4l2h264enc"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges
