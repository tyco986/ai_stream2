import yaml

from utils.base_pipeline.base_rtsp import PIPELINE_YML, BaseRTSPPipeline
from utils.base_pipeline.utils.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.tool.parser.rtmpose_parser import PADDING, RtmposeParser
from utils.tool.logger.det_logger import DetLogger
from utils.tool.messager.det_messager import DetMessager
from utils.tool.preprocessor.rect_expander import RectExpander


class RtmposeRTSPPipeline(BaseRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "nvtracker",
        "sgie0",
        "nvdsanalytics",
        "nvstreamdemux",
        "queue_demux",
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
        self.logger = DetLogger(**self.logger)
        self.parser = RtmposeParser(**self.parser)
        self.messager = DetMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_handler(
            self.rect_expand_target(),
            "rect_expand",
            RectExpander(
                infer_height=self.parser.infer_height,
                infer_width=self.parser.infer_width,
                padding=PADDING,
            ),
        )
        self.attach_detections("rtmpose")
        return self.pipeline
