import copy

from ..base_generator import (
    BaseRTSPGenerator,
    BaseRTSPVisGenerator,
    RTSP_TOPOLOGY_DOC,
    VIS_RTSP_TOPOLOGY_DOC,
)
from ..subelement_generator import YoloSeg


class SegRTSPGenerator(BaseRTSPGenerator):
    GENERATOR = "SegRTSPGenerator"

    f"""Generate YOLO segmentation RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }


class SegVisRTSPGenerator(BaseRTSPVisGenerator):
    GENERATOR = "SegVisRTSPGenerator"

    f"""Generate YOLO segmentation RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def apply_pgie_config(self) -> None:
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": False,
        }
