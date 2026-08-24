from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.utils.validate import validate_probe_interval
from utils.tool.parser.seg_parser import SegParser
from utils.tool.logger.det_logger import DetLogger
from utils.tool.messager.det_messager import DetMessager


class SegRTSPPipeline(BaseRTSPPipeline):
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
        "fakesink",
    )

    def __init__(self, config_dir, pipeline_name, parser=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.parser = parser
        self.logger = logger
        self.logger["times"] = self.SINK_PATHS
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))

    def build(self):
        assert self.pgie_interval == 0, "pgie interval other than 0 is not supported"
        self.logger = DetLogger(**self.logger)
        self.parser = SegParser(**self.parser)
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_detections("yolo")
        return self.pipeline
