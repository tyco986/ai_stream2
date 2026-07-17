import copy

from ..base_generator import (
    BaseSahiRTSPGenerator,
    BaseSahiVisRTSPGenerator,
    SAHI_RTSP_TOPOLOGY_DOC,
    SAHI_VIS_RTSP_TOPOLOGY_DOC,
)
from ..subelement_generator import YoloDetSahi


class DetSahiRTSPGenerator(BaseSahiRTSPGenerator):
    GENERATOR = "DetSahiRTSPGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDetSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config


class DetSahiVisRTSPGenerator(BaseSahiVisRTSPGenerator):
    GENERATOR = "DetSahiVisRTSPGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to disable nvdsanalytics rules. Set ``tracker=None`` to skip nvtracker.
    {SAHI_VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloDetSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
