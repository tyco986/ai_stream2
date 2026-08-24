from utils.base_pipeline.base_image import BaseImagePipeline
from utils.base_pipeline.utils.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.tool.drawer.rtmpose_drawer import PADDING
from utils.tool.drawer.stgcnpp_drawer import INFER_HEIGHT, INFER_WIDTH, StgcnppDrawer
from utils.tool.logger.stgcnpp_logger import StgcnppLogger
from utils.tool.messager.det_messager import DetMessager
from utils.tool.preprocessor.rect_expander import RectExpander


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
        self.logger = StgcnppLogger(**self.logger)
        self.drawer = StgcnppDrawer(**self.drawer)
        self.parser = self.drawer
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_handler(
            "pgie",
            "rect_expand",
            RectExpander(
                infer_height=INFER_HEIGHT,
                infer_width=INFER_WIDTH,
                padding=PADDING,
            ),
        )
        self.attach_detections("stgcnpp")
        return self.pipeline
