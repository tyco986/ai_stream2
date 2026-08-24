import copy

from ..base_generator.base_sahi_vis_video import (
    BaseSahiVisVideoGenerator,
    SAHI_VIS_VIDEO_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiVisVideoGenerator(BaseSahiVisVideoGenerator):
    GENERATOR = "DetSahiVisVideoGenerator"

    f"""Generate YOLO SAHI detection video pipeline YAML with OSD and mp4 filesink.

    {SAHI_VIS_VIDEO_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
