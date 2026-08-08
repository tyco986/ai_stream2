import copy

from ..base_generator.base_video import BaseVideoGenerator, VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_pgie import YoloSeg


class SegVideoGenerator(BaseVideoGenerator):
    GENERATOR = "SegVideoGenerator"

    f"""Generate YOLO segmentation video pipeline YAML.

    Reads ``input`` video via DeepStream, runs inference with OSD, and writes the
    annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
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
