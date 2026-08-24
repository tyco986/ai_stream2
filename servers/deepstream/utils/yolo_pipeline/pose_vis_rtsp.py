from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.utils.validate import validate_probe_interval
from utils.tool.drawer.yolo_pose_fade_drawer import YoloPoseFadeDrawer
from utils.tool.logger.det_logger import DetLogger
from utils.tool.messager.det_messager import DetMessager


class PoseVisRTSPPipeline(BaseRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "nvtracker",
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

    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.logger["times"] = self.SINK_PATHS
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.drawer.get("interval", 0))

    def build(self):
        self.logger = DetLogger(**self.logger)
        self.drawer = YoloPoseFadeDrawer(**self.fade_drawer_params())
        self.parser = self.drawer
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_handler("pgie", "det_cache", self.drawer.cache_detections)
        self.attach_detections("yolo")
        return self.pipeline
