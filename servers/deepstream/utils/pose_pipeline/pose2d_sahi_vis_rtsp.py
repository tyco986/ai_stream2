from utils.pose_pipeline.pose2d_sahi_mixin import Pose2DSahiMixin
from utils.pose_pipeline.pose2d_vis_rtsp import Pose2DVisRTSPPipeline


class Pose2DSahiVisRTSPPipeline(Pose2DSahiMixin, Pose2DVisRTSPPipeline):
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
        "nvosdbin",
        "queue_enc",
        "nvv4l2h264enc",
        "h264parse",
        "rtspclientsink",
    )
