import copy

from .det_rtsp import (
    DetRTSPGenerator,
    DetVisRTSPGenerator,
    RTSP_TOPOLOGY_DOC,
    VIS_RTSP_TOPOLOGY_DOC,
)
from .utils import YoloSeg


class SegRTSPGenerator(DetRTSPGenerator):
    GENERATOR = "SegRTSPGenerator"

    f"""Generate YOLO segmentation RTSP pipeline for event alert + probe-side Kafka.

    Set ``analyzer=None`` to keep nvdsanalytics inserted with master switch off.
    Set ``tracker=None`` to skip nvtracker. Pass ``tracker={{"class_id": ...}}`` to insert nvtracker.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker`` or RTSP preview sink;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {RTSP_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": True,
        }


class SegVisRTSPGenerator(DetVisRTSPGenerator):
    GENERATOR = "SegVisRTSPGenerator"

    f"""Generate YOLO segmentation RTSP pipeline for event alert + probe-side Kafka + live preview.

    Requires ``enable_visualized_rtsp=True``.
    Set ``analyzer=None`` to keep nvdsanalytics inserted with master switch off.
    Set ``tracker=None`` to skip nvtracker. Pass ``tracker={{"class_id": ...}}`` to insert nvtracker. Does not insert ``nvmsgconv`` / ``nvmsgbroker``;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloSeg)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config

    def event_osd_kwargs(self, gpu_id: int) -> dict:
        return {
            "gpu_id": gpu_id,
            "display_bbox": True,
            "display_text": True,
            "display_mask": True,
        }
