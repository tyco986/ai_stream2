import yaml

from utils.base_pipeline.base_video import PIPELINE_YML, BaseVideoPipeline
from utils.base_pipeline.utils.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.tool.parser.rtmpose_parser import PADDING
from utils.tool.parser.stgcnpp_parser import INFER_HEIGHT, INFER_WIDTH, StgcnppParser
from utils.tool.logger.stgcnpp_logger import StgcnppLogger
from utils.tool.messager.det_messager import DetMessager
from utils.tool.preprocessor.rect_expander import RectExpander


class StgcnppVideoPipeline(BaseVideoPipeline):
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
        "nvvideoconvert",
        "fakesink",
    )

    def __init__(self, config_dir, pipeline_name, parser=dict(), logger=dict(), messager=dict()):
        super().__init__(config_dir, pipeline_name)
        self.parser = parser
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
        self.parser = StgcnppParser(**self.parser)
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
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
