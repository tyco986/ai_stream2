import copy

from ..base_generator.base_video import BaseVideoGenerator, VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloSeg


class SegVideoGenerator(BaseVideoGenerator):
    GENERATOR = "SegVideoGenerator"

    f"""Generate YOLO segmentation video pipeline YAML (headless, ends at fakesink).

    {VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
