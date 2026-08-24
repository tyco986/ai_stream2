from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin
from utils.stgcnpp_pipeline.stgcnpp_rtsp import StgcnppRTSPPipeline


class StgcnppSahiRTSPPipeline(RtmposeSahiMixin, StgcnppRTSPPipeline):
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
        "fakesink",
    )
