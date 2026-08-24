from pyservicemaker import Receiver

from utils.base_pipeline.base_rtsp import BaseRTSPPipeline
from utils.base_pipeline.utils.validate import validate_probe_interval
from utils.tool.bridge.presence_capture_state import PresenceCaptureState
from utils.tool.drawer.presence_drawer import PresenceFadeDrawer
from utils.tool.logger.presence_logger import PresenceLogger
from utils.tool.messager.presence_messager import PresenceMessager
from utils.tool.receiver.raw_capturer import RawCapturer
from utils.tool.receiver.vis_capturer import VisCapturer


class PresenceVisRTSPPipeline(BaseRTSPPipeline):
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
        validate_probe_interval(self.pgie_interval, self.drawer.get("interval", 0))
        self.capturer = capturer

    def handle_detections(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
            PresenceCaptureState.update(result)

    def build(self):
        self.logger = PresenceLogger(**self.logger)
        self.drawer = PresenceFadeDrawer(
            drawer=self.fade_drawer_params(), debouncer=self.debouncer
        )
        self.parser = self.drawer
        self.messager = PresenceMessager(**self.messager)
        self.attach_latency_and_times(self.logger)
        self.attach_detections("presence")
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
