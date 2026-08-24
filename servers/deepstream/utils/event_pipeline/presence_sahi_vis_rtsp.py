from utils.event_pipeline.presence_vis_rtsp import PresenceVisRTSPPipeline


class PresenceSahiVisRTSPPipeline(PresenceVisRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvsahipreprocess",
        "pgie",
        "queue_sahi",
        "nvsahipostprocess",
        "nvtracker",
        "nvdsanalytics",
        "nvstreamdemux",
        "queue_demux",
        "nvvideoconvert",
        "tee_raw",
        "queue_osd",
        "nvvideoconvert_osd",
        "capsfilter_osd",
        "nvosdbin",
        "tee_vis",
        "queue_enc",
        "nvv4l2h264enc",
        "h264parse",
        "rtspclientsink",
    )
