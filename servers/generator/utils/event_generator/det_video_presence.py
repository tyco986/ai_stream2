import copy

from ..base_generator.base_event_video import (
    VIDEO_EVENT_TOPOLOGY_DOC,
    BaseEventVideoGenerator,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetVideoPresenceGenerator(BaseEventVideoGenerator):
    GENERATOR = "DetVideoPresenceGenerator"

    f"""Generate YOLO detection video pipeline for event alert + appsink capture.

    Reads ``input`` video via DeepStream, runs inference with event probe-side
    capture branches, and writes the annotated result to ``output``.
    {VIDEO_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
