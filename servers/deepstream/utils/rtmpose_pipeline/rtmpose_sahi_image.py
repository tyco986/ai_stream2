from utils.rtmpose_pipeline.rtmpose_image import RtmposeImagePipeline
from utils.rtmpose_pipeline.rtmpose_sahi_mixin import RtmposeSahiMixin


class RtmposeSahiImagePipeline(RtmposeSahiMixin, RtmposeImagePipeline):
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
        "nvosdbin",
        "nvvideoconvert",
        "nvjpegenc",
        "filesink",
    )
