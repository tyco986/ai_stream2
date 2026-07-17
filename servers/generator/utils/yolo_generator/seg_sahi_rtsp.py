import copy

from ..base_generator import (
    BaseSahiRTSPGenerator,
    BaseSahiVisRTSPGenerator,
    SAHI_RTSP_TOPOLOGY_DOC,
    SAHI_VIS_RTSP_TOPOLOGY_DOC,
)
from ..subelement_generator import YoloSegSahi


class SegSahiRTSPGenerator(BaseSahiRTSPGenerator):
    GENERATOR = "SegSahiRTSPGenerator"

    f"""Generate YOLO SAHI segmentation RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSegSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }


class SegSahiVisRTSPGenerator(BaseSahiVisRTSPGenerator):
    GENERATOR = "SegSahiVisRTSPGenerator"

    f"""Generate YOLO SAHI segmentation RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {SAHI_VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSegSahi)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }
