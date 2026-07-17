import copy

from ..base_generator import (
    BaseRTSPGenerator,
    BaseRTSPVisGenerator,
    RTSP_TOPOLOGY_DOC,
    VIS_RTSP_TOPOLOGY_DOC,
)
from ..subelement_generator import YoloDet


class DetRTSPGenerator(BaseRTSPGenerator):
    GENERATOR = "DetRTSPGenerator"

    f"""Generate YOLO detection RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config


class DetVisRTSPGenerator(BaseRTSPVisGenerator):
    GENERATOR = "DetVisRTSPGenerator"

    f"""Generate YOLO detection RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
