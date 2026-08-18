import copy

from ..base_generator.base_event_sahi_video import (
    BaseEventSahiVideoGenerator,
    SAHI_VIDEO_EVENT_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiVideoPresenceGenerator(BaseEventSahiVideoGenerator):
    GENERATOR = "DetSahiVideoPresenceGenerator"

    f"""Generate YOLO SAHI detection video pipeline for event alert + appsink capture.

    Reads ``input`` video via DeepStream, runs SAHI inference with event capture
    branches, and writes the annotated result to ``output``.
    {SAHI_VIDEO_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
