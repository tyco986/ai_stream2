from .base_sahi_pose import BaseSahiPose
from .base_sahi_vis_video import BaseSahiVisVideoGenerator, SAHI_VIS_VIDEO_TOPOLOGY_DOC

SAHI_POSE_VIS_VIDEO_TOPOLOGY_DOC = SAHI_VIS_VIDEO_TOPOLOGY_DOC.replace(
    "nvsahipostprocess", "nvsahipostprocess_pose"
)


class BaseSahiPoseVisVideoGenerator(BaseSahiPose, BaseSahiVisVideoGenerator):
    SINK_PATH_TEMPLATES = {
        key: [
            "nvsahipostprocess_pose" if name == "nvsahipostprocess" else name
            for name in path
        ]
        for key, path in BaseSahiVisVideoGenerator.SINK_PATH_TEMPLATES.items()
    }

    f"""Generate YOLO SAHI pose video pipeline YAML with OSD and mp4 filesink.

    {SAHI_POSE_VIS_VIDEO_TOPOLOGY_DOC}
    """
