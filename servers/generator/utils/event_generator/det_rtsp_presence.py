import copy

from ..base_generator.base_event_rtsp import (
    RTSP_EVENT_TOPOLOGY_DOC,
    BaseEventRTSPGenerator,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetRTSPPresenceGenerator(BaseEventRTSPGenerator):
    GENERATOR = "DetRTSPPresenceGenerator"

    f"""Generate YOLO detection RTSP pipeline for event alert + appsink capture.

    No RTSP preview sink: capture ends at appsinks only.
    {RTSP_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
