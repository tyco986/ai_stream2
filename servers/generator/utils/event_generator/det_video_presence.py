from ..base_generator import VIDEO_EVENT_TOPOLOGY_DOC, BaseEventVideoGenerator
from ..yolo_generator.det_video import DetVideoGenerator


class DetVideoPresenceGenerator(BaseEventVideoGenerator, DetVideoGenerator):
    GENERATOR = "DetVideoPresenceGenerator"

    f"""Generate YOLO detection video pipeline for event alert + appsink capture.

    Reads ``input`` video via DeepStream, runs inference with event probe-side
    capture branches, and writes the annotated result to ``output``.
    {VIDEO_EVENT_TOPOLOGY_DOC}
    """
