import copy

from ..base_generator.base_event_sahi_vis_video import (
    BaseEventSahiVisVideoGenerator,
    SAHI_VIS_VIDEO_EVENT_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiVisVideoPresenceGenerator(BaseEventSahiVisVideoGenerator):
    GENERATOR = "DetSahiVisVideoPresenceGenerator"

    f"""Generate YOLO SAHI vis-video pipeline for event alert + nvcapturer dump.

    {SAHI_VIS_VIDEO_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
