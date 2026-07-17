from ..base_generator import (
    BaseEventSahiVisRTSPGenerator,
    SAHI_VIS_RTSP_EVENT_TOPOLOGY_DOC,
)
from ..yolo_generator.det_sahi_rtsp import DetSahiVisRTSPGenerator


class DetSahiRTSPPresenceGenerator(BaseEventSahiVisRTSPGenerator, DetSahiVisRTSPGenerator):
    GENERATOR = "DetSahiRTSPPresenceGenerator"

    f"""Generate YOLO SAHI detection RTSP pipeline for event alert + appsink capture.

    Per-stream branches tee raw/vis appsinks and continue encode to ``rtspclientsink``.
    {SAHI_VIS_RTSP_EVENT_TOPOLOGY_DOC}
    """
