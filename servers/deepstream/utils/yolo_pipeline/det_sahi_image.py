from utils.base_pipeline.base_image import BaseImagePipeline
from utils.base_pipeline.utils.validate import validate_probe_interval
from utils.tool.drawer.det_drawer import DetDrawer
from utils.tool.logger.det_logger import DetLogger
from utils.tool.messager.det_messager import DetMessager


class DetSahiImagePipeline(BaseImagePipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvsahipreprocess",
        "pgie",
        "queue_sahi",
        "nvsahipostprocess",
        "nvdsanalytics",
        "nvosdbin",
        "nvvideoconvert",
        "nvjpegenc",
        "filesink",
    )

    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.logger["times"] = self.SINK_PATHS
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        self.logger = DetLogger(**self.logger)
        self.drawer = DetDrawer(**self.drawer)
        self.parser = self.drawer
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_detections("yolo")
        return self.pipeline
