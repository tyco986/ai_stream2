from .base_sahi_image import BaseSahiImageGenerator, SAHI_IMAGE_TOPOLOGY_DOC
from .base_sahi_pose import BaseSahiPose

SAHI_POSE_IMAGE_TOPOLOGY_DOC = SAHI_IMAGE_TOPOLOGY_DOC.replace(
    "nvsahipostprocess", "nvsahipostprocess_pose"
)


class BaseSahiPoseImageGenerator(BaseSahiPose, BaseSahiImageGenerator):
    SINK_PATH_TEMPLATES = {
        key: [
            "nvsahipostprocess_pose" if name == "nvsahipostprocess" else name
            for name in path
        ]
        for key, path in BaseSahiImageGenerator.SINK_PATH_TEMPLATES.items()
    }

    f"""Generate YOLO SAHI pose image pipeline YAML.

    Reads ``input`` image via DeepStream, runs SAHI pose inference with OSD,
    and writes the annotated result to ``output``. Does not insert ``nvmsgconv`` / ``nvmsgbroker``.
    {SAHI_POSE_IMAGE_TOPOLOGY_DOC}
    """
