from pyservicemaker import Probe

from utils.base_pipeline.base_video import BaseVideoPipeline
from utils.base_pipeline.validate import validate_probe_interval
from utils.probe.seg_video_probe import SegVideoProbe
from utils.probe.utils.logger.det_logger import DetLogger


class SegVideoPipeline(BaseVideoPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "nvtracker",
        "nvdsanalytics",
        "nvosdbin",
        "nvvideoconvert",
        "nvv4l2h264enc",
        "h264parse",
        "mp4mux",
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
        assert self.pgie_interval == 0, "pgie interval other than 0 is not supported"
        logger = DetLogger(**self.logger)
        self.attach_latency_and_times(logger)
        self.pipeline.attach(
            "nvdsanalytics",
            Probe(
                "yolo",
                SegVideoProbe(drawer=self.drawer, logger=logger, messager=self.messager),
            ),
        )
        return self.pipeline
