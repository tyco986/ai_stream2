import copy

from ..base_generator.base_sahi_rtsp import BaseSahiRTSPGenerator, SAHI_RTSP_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiRTSPGenerator(BaseSahiRTSPGenerator):
    GENERATOR = "DetSahiRTSPGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
