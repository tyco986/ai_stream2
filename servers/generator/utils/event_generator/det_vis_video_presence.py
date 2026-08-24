import copy

from ..base_generator.base_event_vis_video import (
    VIS_VIDEO_EVENT_TOPOLOGY_DOC,
    BaseEventVisVideoGenerator,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetVisVideoPresenceGenerator(BaseEventVisVideoGenerator):
    GENERATOR = "DetVisVideoPresenceGenerator"

    f"""Generate YOLO detection vis-video pipeline for event alert + appsink capture.

    {VIS_VIDEO_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
