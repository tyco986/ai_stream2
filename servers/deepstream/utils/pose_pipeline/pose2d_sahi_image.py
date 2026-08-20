from utils.pose_pipeline.pose2d_sahi_mixin import Pose2DSahiMixin
from utils.pose_pipeline.pose2d_image import Pose2DImagePipeline


class Pose2DSahiImagePipeline(Pose2DSahiMixin, Pose2DImagePipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvsahipreprocess",
        "pgie",
        "queue_sahi",
        "nvsahipostprocess",
        "sgie0",
        "nvdsanalytics",
        "nvosdbin",
        "nvvideoconvert",
        "nvjpegenc",
        "filesink",
    )
