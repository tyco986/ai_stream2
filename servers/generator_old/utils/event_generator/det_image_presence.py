import copy

from ..base_generator.base_event_image import (
    IMAGE_EVENT_TOPOLOGY_DOC,
    BaseEventImageGenerator,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetImagePresenceGenerator(BaseEventImageGenerator):
    GENERATOR = "DetImagePresenceGenerator"

    f"""Generate YOLO detection image pipeline for event alert + nvcapturer dump.

    {IMAGE_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
