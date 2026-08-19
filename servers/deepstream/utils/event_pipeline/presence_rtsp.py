from pyservicemaker import Receiver

from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.validate import validate_probe_interval
from utils.probe.det_vis_rtsp_presence_probe import DetVisRTSPPresenceProbe
from utils.receiver.raw_capturer import RawCapturer
from utils.receiver.vis_capturer import VisCapturer
from utils.probe.utils.logger.presence_logger import PresenceLogger


class PresenceRTSPPipeline(BaseRTSPPipeline):
    SINK_PATHS = (
        "latency",
        "nvurisrcbin",
        "nvstreammux",
        "pgie",
        "nvtracker",
        "nvdsanalytics",
        "nvstreamdemux",
        "queue_demux",
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
        "rtspclientsink",
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
        self.attach_nvdsanalytics_probe(
            "presence",
            DetVisRTSPPresenceProbe(
                debouncer=self.debouncer,
                drawer=self.drawer,
                logger=logger,
                messager=self.messager,
            ),
        )
        for index in self.stream_indices():
            self.pipeline.attach(
                f"appsink_raw{index}",
                Receiver(f"raw{index}", RawCapturer(self.capturer)),
                tips="new-sample",
            )
            self.pipeline.attach(
                f"appsink_vis{index}",
                Receiver(f"vis{index}", VisCapturer(self.capturer)),
                tips="new-sample",
            )
        return self.pipeline
