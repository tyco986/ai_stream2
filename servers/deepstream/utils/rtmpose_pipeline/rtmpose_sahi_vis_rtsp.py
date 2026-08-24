from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin
from utils.rtmpose_pipeline.rtmpose_vis_rtsp import RtmposeVisRTSPPipeline


class RtmposeSahiVisRTSPPipeline(RtmposeSahiMixin, RtmposeVisRTSPPipeline):
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
