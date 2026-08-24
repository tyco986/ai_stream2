from utils.event_pipeline.presence_vis_video import PresenceVisVideoPipeline


class PresenceSahiVisVideoPipeline(PresenceVisVideoPipeline):
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
        "mp4mux",
        "filesink",
    )
