import copy

from ..base_generator.base_sahi_video import BaseSahiVideoGenerator, SAHI_VIDEO_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloSeg


class SegSahiVideoGenerator(BaseSahiVideoGenerator):
    GENERATOR = "SegSahiVideoGenerator"

    f"""Generate YOLO SAHI segmentation video pipeline YAML (headless, ends at fakesink).

    {SAHI_VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
