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

    Set ``analyzer=None`` to skip nvdsanalytics. ``tracker`` is required for seg fade draw.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker`` or RTSP preview sink;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        analyzer: dict | None,
        pgie: dict,
        tracker: dict = {"class_id": -1},
        interval: int = 0,
    ) -> None:
        assert tracker is not None, "seg task requires tracker"
        super().__init__(
            streams=streams,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
            interval=interval,
        )

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
    Set ``analyzer=None`` to skip nvdsanalytics. ``tracker`` is required for seg fade draw.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker``;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def __init__(
        self,
        streams: dict[str, dict],
        enable_visualized_rtsp: bool,
        analyzer: dict | None,
        pgie: dict,
        tracker: dict = {"class_id": 0},
        interval: int = 0,
    ) -> None:
        assert tracker is not None, "seg task requires tracker"
        super().__init__(
            streams=streams,
            enable_visualized_rtsp=enable_visualized_rtsp,
            analyzer=analyzer,
            pgie=pgie,
            tracker=tracker,
            interval=interval,
        )

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
