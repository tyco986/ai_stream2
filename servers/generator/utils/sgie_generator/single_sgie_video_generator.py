from pathlib import Path

from ..base_generator.base_video import BaseVideoGenerator
from ..subelement_generator.pipeline import TRACKER_LL_LIB
from .single_sgie_mixin import SingleSgieMixin

SINGLE_SGIE_VIDEO_TOPOLOGY_DOC = """
    Topology::

        nvurisrcbin → nvstreammux → nvinfer → nvtracker → sgie → nvdsanalytics → nvosdbin
            → nvvideoconvert → nvv4l2h264enc → h264parse → mp4mux → filesink
"""


class SingleSgieVideoGenerator(SingleSgieMixin, BaseVideoGenerator):
    GENERATOR = "SingleSgieVideoGenerator"
    SINK_PATH_TEMPLATES = {
        "filesink": [
            "nvurisrcbin",
            "nvstreammux",
            "nvinfer",
            "nvtracker",
            "sgie",
            "nvdsanalytics",
            "nvosdbin",
            "nvvideoconvert",
            "nvv4l2h264enc",
            "h264parse",
            "mp4mux",
            "filesink",
        ],
    }

    f"""Generate single-SGIE video pipeline YAML.

    {SINGLE_SGIE_VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        sgie: dict,
        tracker: dict | None = None,
        interval: int = 0,
    ) -> None:
        self.sgie = sgie
        super().__init__(
            input=input,
            output=output,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
            interval=interval,
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
            "nvinfer",
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
            "nvstreammux": "nvinfer",
        }
        inference_tail = "nvinfer"
        if self.enable_nvtracker:
            edges[inference_tail] = "nvtracker"
            inference_tail = "nvtracker"
        edges[inference_tail] = "sgie"
        edges["sgie"] = "nvdsanalytics"
        edges["nvdsanalytics"] = "nvosdbin"
        edges["nvosdbin"] = "nvvideoconvert"
        edges["nvvideoconvert"] = "nvv4l2h264enc"
        edges["nvv4l2h264enc"] = "h264parse"
        edges["h264parse"] = "mp4mux"
        edges["mp4mux"] = "filesink"
        self.pipeline["deepstream"]["edges"] = edges
