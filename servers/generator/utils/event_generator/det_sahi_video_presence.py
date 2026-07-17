from ..base_generator import (
    BaseEventSahiVideoGenerator,
    SAHI_VIDEO_EVENT_TOPOLOGY_DOC,
)
from ..yolo_generator.det_sahi_video import DetSahiVideoGenerator


class DetSahiVideoPresenceGenerator(BaseEventSahiVideoGenerator, DetSahiVideoGenerator):
    GENERATOR = "DetSahiVideoPresenceGenerator"

    f"""Generate YOLO SAHI detection video pipeline for event alert + appsink capture.

    Reads ``input`` video via DeepStream, runs SAHI inference with event capture
    branches, and writes the annotated result to ``output``.
    {SAHI_VIDEO_EVENT_TOPOLOGY_DOC}
    """
