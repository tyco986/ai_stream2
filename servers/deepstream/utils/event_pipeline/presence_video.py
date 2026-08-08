from pyservicemaker import Probe, Receiver

from utils.base_pipeline.base_video import BaseVideoPipeline
from utils.base_pipeline.validate import validate_probe_interval
from utils.probe.det_presence_video_probe import DetPresenceVideoProbe
from utils.receiver.raw_capturer import RawCapturer
from utils.receiver.vis_capturer import VisCapturer
from utils.probe.utils.logger.presence_logger import PresenceLogger


class PresenceVideoPipeline(BaseVideoPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "nvinfer",
        "nvtracker",
        "nvdsanalytics",
        "nvvideoconvert",
        "tee_raw",
        "queue_osd",
        "nvvideoconvert_osd",
        "capsfilter_osd",
        "nvosdbin",
        "tee_vis",
        "queue_enc",
        "nvv4l2h264enc",
        "h264parse",
        "mp4mux",
        "filesink",
    )

    def __init__(
        self,
        config_dir,
        pipeline_name,
        debouncer=dict(),
        drawer=dict(),
        logger=dict(),
        messager=dict(),
        capturer=dict(),
    ):
        super().__init__(config_dir, pipeline_name)
        self.debouncer = debouncer
        self.drawer = drawer
        self.logger = logger
        self.logger["times"] = self.SINK_PATHS
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))
        self.capturer = capturer

    def build(self):
        logger = PresenceLogger(**self.logger)
        self.attach_latency_and_times(logger)
        self.pipeline.attach(
            "nvdsanalytics",
            Probe(
                "presence",
                DetPresenceVideoProbe(
                    debouncer=self.debouncer,
                    drawer=self.drawer,
                    logger=logger,
                    messager=self.messager,
                ),
            ),
        )
        self.pipeline.attach(
            "appsink_raw0",
            Receiver("raw0", RawCapturer(self.capturer)),
            tips="new-sample",
        )
        self.pipeline.attach(
            "appsink_vis0",
            Receiver("vis0", VisCapturer(self.capturer)),
            tips="new-sample",
        )
        return self.pipeline
