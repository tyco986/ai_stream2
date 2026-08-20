from utils.pose_pipeline.pose2d_sahi_mixin import Pose2DSahiMixin
from utils.pose_pipeline.pose2d_rtsp import Pose2DRTSPPipeline


class Pose2DSahiRTSPPipeline(Pose2DSahiMixin, Pose2DRTSPPipeline):
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
        "nvstreamdemux",
        "queue_demux",
        "nvvideoconvert",
        "fakesink",
    )
