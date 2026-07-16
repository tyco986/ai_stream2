import copy

from .det_image import DetImageGenerator
from .det_rtsp import RTSP_TOPOLOGY_DOC, VIS_RTSP_TOPOLOGY_DOC
from .det_sahi_rtsp import DetSahiRTSPGenerator, DetSahiVisRTSPGenerator, SAHI_RTSP_TOPOLOGY_DOC
from .utils import YoloSegSahi


class SegSahiRTSPGenerator(DetSahiRTSPGenerator):
    GENERATOR = "SegSahiRTSPGenerator"

    f"""Generate YOLO SAHI segmentation RTSP pipeline for event alert + probe-side Kafka.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker`` or RTSP preview sink;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {RTSP_TOPOLOGY_DOC}
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        DetImageGenerator.init_pgie(self)
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


class SegSahiVisRTSPGenerator(DetSahiVisRTSPGenerator):
    GENERATOR = "SegSahiVisRTSPGenerator"

    f"""Generate YOLO SAHI segmentation RTSP pipeline for event alert + probe-side Kafka + live preview.

    Requires ``enable_visualized_rtsp=True``.
    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker``;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {VIS_RTSP_TOPOLOGY_DOC}
    {SAHI_RTSP_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        DetImageGenerator.init_pgie(self)
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
