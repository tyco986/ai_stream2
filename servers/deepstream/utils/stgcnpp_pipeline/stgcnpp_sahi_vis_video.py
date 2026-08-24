from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin
from utils.stgcnpp_pipeline.stgcnpp_vis_video import StgcnppVisVideoPipeline


class StgcnppSahiVisVideoPipeline(RtmposeSahiMixin, StgcnppVisVideoPipeline):
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
        "nvosdbin",
        "nvvideoconvert",
        "nvv4l2h264enc",
        "h264parse",
        "mp4mux",
        "filesink",
    )
