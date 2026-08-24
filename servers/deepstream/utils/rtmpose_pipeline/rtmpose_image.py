from utils.base_pipeline.base_image import BaseImagePipeline
from utils.base_pipeline.utils.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.tool.drawer.rtmpose_drawer import PADDING, RtmposeDrawer
from utils.tool.logger.det_logger import DetLogger
from utils.tool.messager.det_messager import DetMessager
from utils.tool.preprocessor.rect_expander import RectExpander


class RtmposeImagePipeline(BaseImagePipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "sgie0",
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

    def rect_expand_target(self):
        target = "pgie"
        return target

    def build(self):
        self.logger = DetLogger(**self.logger)
        self.drawer = RtmposeDrawer(**self.drawer)
        self.parser = self.drawer
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_handler(
            self.rect_expand_target(),
            "rect_expand",
            RectExpander(
                infer_height=self.drawer.infer_height,
                infer_width=self.drawer.infer_width,
                padding=PADDING,
            ),
        )
        self.attach_detections("rtmpose")
        return self.pipeline
