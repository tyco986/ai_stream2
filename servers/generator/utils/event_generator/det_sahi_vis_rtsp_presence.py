import copy

from ..base_generator.base_event_sahi_vis_rtsp import (
    BaseEventSahiVisRTSPGenerator,
    SAHI_VIS_RTSP_EVENT_TOPOLOGY_DOC,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiVisRTSPPresenceGenerator(BaseEventSahiVisRTSPGenerator):
    GENERATOR = "DetSahiVisRTSPPresenceGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline for event alert + appsink capture.

    Per-stream branches tee raw/vis appsinks and continue encode to ``rtspclientsink``.
    {SAHI_VIS_RTSP_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
