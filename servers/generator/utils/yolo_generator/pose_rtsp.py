import copy

from .det_rtsp import (
    DetRTSPGenerator,
    DetVisRTSPGenerator,
    RTSP_TOPOLOGY_DOC,
    VIS_RTSP_TOPOLOGY_DOC,
)
from .utils import YoloPose


class PoseRTSPGenerator(DetRTSPGenerator):
    GENERATOR = "PoseRTSPGenerator"

    f"""Generate YOLO pose RTSP pipeline for event alert + probe-side Kafka.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    Does not insert ``nvmsgconv`` / ``nvmsgbroker`` or RTSP preview sink;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {RTSP_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config


class PoseVisRTSPGenerator(DetVisRTSPGenerator):
    GENERATOR = "PoseVisRTSPGenerator"

    f"""Generate YOLO pose RTSP pipeline for event alert + probe-side Kafka + live preview.

    Requires ``enable_visualized_rtsp=True``.
    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker. Does not insert ``nvmsgconv`` / ``nvmsgbroker``;
    DeepStream attaches ``BaseProbe`` on ``analyzer`` for ``EventMessager`` and appsink
    capture branches.
    {VIS_RTSP_TOPOLOGY_DOC}
    """

    def init_pgie(self) -> None:
        super().init_pgie()
        self.pgie_generator.config = copy.deepcopy(YoloPose)
        self.pgie_generator.update_config()
        self.pgie_yml = self.pgie_generator.config
