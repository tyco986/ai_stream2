from pyservicemaker import Probe

from utils.base_pipeline.base_image import BaseImagePipeline
from utils.base_pipeline.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.probe.rect_expand_probe import RectExpandProbe
from utils.probe.stgcnpp_image_probe import StgcnppImageProbe
from utils.probe.utils.drawer.pose2d_drawer import PADDING
from utils.probe.utils.drawer.stgcnpp_drawer import INFER_HEIGHT, INFER_WIDTH
from utils.probe.utils.logger.stgcnpp_logger import StgcnppLogger


class StgcnppImagePipeline(BaseImagePipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "sgie0",
        "nvdspreprocess",
        "sgie1",
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
            "pgie",
            Probe(
                "rect_expand",
                RectExpandProbe(
                    infer_height=INFER_HEIGHT,
                    infer_width=INFER_WIDTH,
                    padding=PADDING,
                ),
            ),
        )
        self.pipeline.attach(
            "nvdsanalytics",
            Probe(
                "stgcnpp",
                StgcnppImageProbe(drawer=self.drawer, logger=logger, messager=self.messager),
            ),
        )
        return self.pipeline
