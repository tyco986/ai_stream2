from utils.rtmpose_pipeline.rtmpose_rtsp import RtmposeRTSPPipeline
from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin


class RtmposeSahiRTSPPipeline(RtmposeSahiMixin, RtmposeRTSPPipeline):
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
