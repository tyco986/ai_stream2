import copy

from pathlib import Path

from .det_video import DetVideoGenerator, VIDEO_TOPOLOGY_DOC
from .utils import YoloSeg

class SegVideoGenerator(DetVideoGenerator):
    GENERATOR = "SegVideoGenerator"

    f"""Generate YOLO segmentation video pipeline YAML.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. ``tracker`` is required for seg fade draw.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIDEO_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        input: str | Path,
        output: str | Path,
        analyzer: dict | None,
        pgie: dict,
        tracker: dict = {"class_id": -1},
        interval: int = 0,
    ) -> None:
        assert tracker is not None, "seg task requires tracker"
        super().__init__(
            input=input,
            output=output,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
            interval=interval,
        )

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": True,
        }
