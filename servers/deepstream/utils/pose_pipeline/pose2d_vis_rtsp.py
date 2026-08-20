import yaml
from pyservicemaker import Probe

from utils.base_pipeline.base_rtsp import PIPELINE_YML, BaseRTSPPipeline
from utils.base_pipeline.validate import (
    sgie_period_from_config,
    validate_probe_interval,
    validate_sgie_interval,
)
from utils.probe.det_fade_cache_probe import DetFadeCacheProbe
from utils.probe.pose2d_vis_rtsp_probe import Pose2DVisRTSPProbe
from utils.probe.rect_expand_probe import RectExpandProbe
from utils.probe.utils.drawer.pose2d_drawer import PADDING
from utils.probe.utils.drawer.pose2d_fade_drawer import Pose2DFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger


class Pose2DVisRTSPPipeline(BaseRTSPPipeline):
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
        sgie_interval = sgie_period_from_config(self.config_dir)
        validate_probe_interval(
            self.pgie_interval, self.messager.get("interval", 0), sgie_interval
        )
        validate_probe_interval(
            self.pgie_interval, self.logger.get("interval", 0), sgie_interval
        )
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
        logger = DetLogger(**self.logger)
        drawer = Pose2DFadeDrawer(**self.drawer)
        self.attach_latency_and_times(logger)
        self.pipeline.attach(
            self.cache_target(),
            Probe("det_cache", DetFadeCacheProbe(drawer)),
        )
        self.pipeline.attach(
            self.rect_expand_target(),
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
                drawer=drawer,
                logger=logger,
                messager=self.messager,
            ),
        )
        return self.pipeline
