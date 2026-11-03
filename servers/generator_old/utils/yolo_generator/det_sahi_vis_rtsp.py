import copy

from ..base_generator.base_sahi_rtsp_vis import BaseSahiVisRTSPGenerator, SAHI_VIS_RTSP_TOPOLOGY_DOC
from ..subelement_generator.utils.default_gie import YoloDet


class DetSahiVisRTSPGenerator(BaseSahiVisRTSPGenerator):
    GENERATOR = "DetSahiVisRTSPGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {SAHI_VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDet)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
