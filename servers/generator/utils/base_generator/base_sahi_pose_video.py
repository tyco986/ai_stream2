from .base_sahi_pose import BaseSahiPose
from .base_sahi_video import BaseSahiVideoGenerator, SAHI_VIDEO_TOPOLOGY_DOC

SAHI_POSE_VIDEO_TOPOLOGY_DOC = SAHI_VIDEO_TOPOLOGY_DOC.replace(
    "nvsahipostprocess", "nvsahipostprocess_pose"
)


class BaseSahiPoseVideoGenerator(BaseSahiPose, BaseSahiVideoGenerator):
    SINK_PATH_TEMPLATES = {
        key: [
            "nvsahipostprocess_pose" if name == "nvsahipostprocess" else name
            for name in path
        ]
        for key, path in BaseSahiVideoGenerator.SINK_PATH_TEMPLATES.items()
    }

    f"""Generate YOLO SAHI pose video pipeline YAML (headless, ends at fakesink).

    {SAHI_POSE_VIDEO_TOPOLOGY_DOC}
    """
