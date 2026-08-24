import yaml

from utils.base_pipeline.base_video import PIPELINE_YML, BaseVideoPipeline
from utils.base_pipeline.utils.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.tool.drawer.rtmpose_drawer import PADDING
from utils.tool.drawer.stgcnpp_drawer import FADE_INTERVAL, INFER_HEIGHT, INFER_WIDTH
from utils.tool.drawer.stgcnpp_fade_drawer import StgcnppFadeDrawer
from utils.tool.logger.stgcnpp_logger import StgcnppLogger
from utils.tool.messager.det_messager import DetMessager
from utils.tool.preprocessor.rect_expander import RectExpander


class StgcnppVisVideoPipeline(BaseVideoPipeline):
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
        sgie_interval = sgie_period_from_config(self.config_dir)
        validate_probe_interval(
            self.pgie_interval, self.messager.get("interval", 0), sgie_interval
        )
        validate_probe_interval(
            self.pgie_interval, self.logger.get("interval", 0), sgie_interval
        )
        validate_probe_interval(self.pgie_interval, FADE_INTERVAL, sgie_interval)
        validate_sgie_interval(self.pgie_interval, sgie_interval)

    def cache_target(self):
        return "pgie"

    def rect_expand_target(self):
        pipeline = yaml.safe_load(
            (self.config_dir / PIPELINE_YML).read_text(encoding="utf-8")
        )
        names = {node["name"] for node in pipeline["deepstream"]["nodes"]}
        target = "pgie"
        if "nvtracker" in names:
            target = "nvtracker"
        return target

    def build(self):
        self.logger = StgcnppLogger(**self.logger)
        self.drawer = StgcnppFadeDrawer(**self.fade_drawer_params())
        self.parser = self.drawer
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_handler(self.cache_target(), "det_cache", self.drawer.cache_detections)
        self.attach_handler(
            self.rect_expand_target(),
            "rect_expand",
            RectExpander(
                infer_height=INFER_HEIGHT,
                infer_width=INFER_WIDTH,
                padding=PADDING,
            ),
        )
        self.attach_detections("stgcnpp")
        return self.pipeline
