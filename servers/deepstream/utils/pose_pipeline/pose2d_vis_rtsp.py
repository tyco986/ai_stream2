from pyservicemaker import Probe

from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.validate import validate_probe_interval
from utils.probe.pose2d_vis_rtsp_probe import Pose2DVisRTSPProbe
from utils.probe.rect_expand_probe import RectExpandProbe
from utils.probe.utils.drawer.pose2d_drawer import PADDING
from utils.probe.utils.logger.det_logger import DetLogger


class Pose2DVisRTSPPipeline(BaseRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvinfer",
        "nvtracker",
        "sgie",
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

    def build(self):
        logger = DetLogger(**self.logger)
        self.attach_latency_and_times(logger)
        self.pipeline.attach(
            "nvtracker",
            Probe(
                "rect_expand",
                RectExpandProbe(
                    infer_height=self.drawer.get("infer_height", 256),
                    infer_width=self.drawer.get("infer_width", 192),
                    padding=PADDING,
                ),
            ),
        )
        self.attach_nvdsanalytics_probe(
            "pose2d",
            Pose2DVisRTSPProbe(
                drawer=self.drawer,
                logger=logger,
                messager=self.messager,
            ),
        )
        return self.pipeline
