import copy

from ..base_generator.base_event_vis_rtsp import (
    VIS_RTSP_EVENT_TOPOLOGY_DOC,
    BaseEventVisRTSPGenerator,
)
from ..subelement_generator.utils.default_gie import YoloDet


class DetVisRTSPPresenceGenerator(BaseEventVisRTSPGenerator):
    GENERATOR = "DetVisRTSPPresenceGenerator"

    f"""Generate YOLO detection RTSP pipeline for event alert + appsink capture.

    Per-stream branches tee raw/vis appsinks and continue encode to ``rtspclientsink``.
    {VIS_RTSP_EVENT_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
