import copy

from ..base_generator.base_event_sahi_rtsp import (
    SAHI_RTSP_EVENT_TOPOLOGY_DOC,
    BaseEventSahiRTSPGenerator,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiRTSPPresenceGenerator(BaseEventSahiRTSPGenerator):
    GENERATOR = "DetSahiRTSPPresenceGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline for event alert + appsink capture.

    No RTSP preview sink: capture ends at appsinks only.
    {SAHI_RTSP_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
