from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.validate import validate_probe_interval
from utils.probe.seg_sahi_vis_rtsp_probe import SegSahiVisRTSPProbe
from utils.probe.utils.logger.det_logger import DetLogger


class SegSahiVisRTSPPipeline(BaseRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvsahipreprocess",
        "nvinfer",
        "queue_sahi",
        "nvsahipostprocess",
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

    def build(self):
        logger = DetLogger(**self.logger)
        self.attach_latency_and_times(logger)
        self.attach_nvdsanalytics_probe(
            "yolo",
            SegSahiVisRTSPProbe(
                drawer=self.drawer,
                logger=logger,
                messager=self.messager,
            ),
        )
        return self.pipeline
