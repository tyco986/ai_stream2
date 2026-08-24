from utils.event_pipeline.presence_rtsp import PresenceRTSPPipeline


class PresenceSahiRTSPPipeline(PresenceRTSPPipeline):
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
    )
