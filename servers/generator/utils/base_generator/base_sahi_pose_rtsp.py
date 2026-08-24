from .base_sahi_pose import BaseSahiPose
from .base_sahi_rtsp import BaseSahiRTSPGenerator, SAHI_RTSP_TOPOLOGY_DOC

SAHI_POSE_RTSP_TOPOLOGY_DOC = SAHI_RTSP_TOPOLOGY_DOC.replace(
    "nvsahipostprocess", "nvsahipostprocess_pose"
)


class BaseSahiPoseRTSPGenerator(BaseSahiPose, BaseSahiRTSPGenerator):
    SINK_PATH_TEMPLATES = {
        key: [
            "nvsahipostprocess_pose" if name == "nvsahipostprocess" else name
            for name in path
        ]
        for key, path in BaseSahiRTSPGenerator.SINK_PATH_TEMPLATES.items()
    }

    f"""Generate YOLO SAHI pose RTSP pipeline (headless, ends at fakesink).

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {SAHI_POSE_RTSP_TOPOLOGY_DOC}
    """
