from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin
from utils.stgcnpp_pipeline.stgcnpp_vis_rtsp import StgcnppVisRTSPPipeline


class StgcnppSahiVisRTSPPipeline(RtmposeSahiMixin, StgcnppVisRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvsahipreprocess",
        "pgie",
        "queue_sahi",
        "nvsahipostprocess",
        "nvtracker",
        "sgie0",
        "nvdspreprocess",
        "sgie1",
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
