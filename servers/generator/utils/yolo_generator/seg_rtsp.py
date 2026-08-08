import copy

from ..base_generator.base_rtsp import BaseRTSPGenerator, RTSP_TOPOLOGY_DOC
from ..subelement_generator.utils.default_pgie import YoloSeg


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
