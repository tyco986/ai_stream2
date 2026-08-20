from pyservicemaker import Probe

from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.probe.rect_expand_probe import RectExpandProbe
from utils.probe.stgcnpp_rtsp_probe import StgcnppRTSPProbe
from utils.probe.utils.drawer.pose2d_drawer import PADDING
from utils.probe.utils.drawer.stgcnpp_drawer import INFER_HEIGHT, INFER_WIDTH
from utils.probe.utils.logger.stgcnpp_logger import StgcnppLogger


class StgcnppRTSPPipeline(BaseRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "nvtracker",
        "sgie0",
        "nvdspreprocess",
        "sgie1",
        "nvdsanalytics",
        "nvstreamdemux",
        "queue_demux",
        "nvvideoconvert",
        "fakesink",
    )

    def __init__(self, config_dir, pipeline_name, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.drawer = drawer
        self.logger = logger
        self.logger["times"] = self.SINK_PATHS
        self.messager = messager
        sgie_interval = sgie_period_from_config(self.config_dir)
        validate_probe_interval(
            self.pgie_interval, self.messager.get("interval", 0), sgie_interval
        )
        validate_probe_interval(
            self.pgie_interval, self.logger.get("interval", 0), sgie_interval
        )
        validate_sgie_interval(self.pgie_interval, sgie_interval)

    def build(self):
        logger = StgcnppLogger(**self.logger)
        self.attach_latency_and_times(logger)
        self.pipeline.attach(
            "nvtracker",
            Probe(
                "rect_expand",
                RectExpandProbe(
                    infer_height=INFER_HEIGHT,
                    infer_width=INFER_WIDTH,
                    padding=PADDING,
                ),
            ),
        )
        self.attach_nvdsanalytics_probe(
            "stgcnpp",
            StgcnppRTSPProbe(drawer=self.drawer, logger=logger, messager=self.messager),
        )
        return self.pipeline
