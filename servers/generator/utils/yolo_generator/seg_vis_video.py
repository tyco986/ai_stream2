import copy

from ..base_generator.base_vis_video import BaseVisVideoGenerator, VIS_VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloSeg


class SegVisVideoGenerator(BaseVisVideoGenerator):
    GENERATOR = "SegVisVideoGenerator"

    f"""Generate YOLO segmentation video pipeline YAML with OSD and mp4 filesink.

    {VIS_VIDEO_TOPOLOGY_DOC}
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
