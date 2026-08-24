from .base_sahi_pose import BaseSahiPose
from .base_sahi_rtsp_vis import BaseSahiVisRTSPGenerator, SAHI_VIS_RTSP_TOPOLOGY_DOC

SAHI_POSE_VIS_RTSP_TOPOLOGY_DOC = SAHI_VIS_RTSP_TOPOLOGY_DOC.replace(
    "nvsahipostprocess", "nvsahipostprocess_pose"
)


class BaseSahiPoseVisRTSPGenerator(BaseSahiPose, BaseSahiVisRTSPGenerator):
    SINK_PATH_TEMPLATES = {
        key: [
            "nvsahipostprocess_pose" if name == "nvsahipostprocess" else name
            for name in path
        ]
        for key, path in BaseSahiVisRTSPGenerator.SINK_PATH_TEMPLATES.items()
    }

    f"""Generate YOLO SAHI pose RTSP pipeline with OSD preview sink.

    Set ``analyzer=None`` to skip nvdsanalytics. Set ``tracker=None`` to skip nvtracker.
    {SAHI_POSE_VIS_RTSP_TOPOLOGY_DOC}
    """
