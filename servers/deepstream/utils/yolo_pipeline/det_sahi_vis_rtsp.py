from pyservicemaker import Probe

from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.validate import validate_probe_interval
from utils.probe.det_fade_cache_probe import DetFadeCacheProbe
from utils.probe.det_sahi_vis_rtsp_probe import DetSahiVisRTSPProbe
from utils.probe.utils.drawer.det_fade_drawer import DetFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger


class DetSahiVisRTSPPipeline(BaseRTSPPipeline):
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
        drawer = DetFadeDrawer(**self.drawer)
        self.attach_latency_and_times(logger)
        self.pipeline.attach(
            "nvsahipostprocess",
            Probe("det_cache", DetFadeCacheProbe(drawer)),
        )
        self.attach_nvdsanalytics_probe(
            "yolo",
            DetSahiVisRTSPProbe(
                drawer=drawer,
                logger=logger,
                messager=self.messager,
            ),
        )
        return self.pipeline
