from utils.pose_pipeline.pose2d_sahi_mixin import Pose2DSahiMixin
from utils.pose_pipeline.pose2d_video import Pose2DVideoPipeline


class Pose2DSahiVideoPipeline(Pose2DSahiMixin, Pose2DVideoPipeline):
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
        "nvv4l2h264enc",
        "h264parse",
        "mp4mux",
        "filesink",
    )
