from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin
from utils.rtmpose_pipeline.rtmpose_video import RtmposeVideoPipeline


class RtmposeSahiVideoPipeline(RtmposeSahiMixin, RtmposeVideoPipeline):
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
        "nvdsanalytics",
        "nvvideoconvert",
        "fakesink",
    )
