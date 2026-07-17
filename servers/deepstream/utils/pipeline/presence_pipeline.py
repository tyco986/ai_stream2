from pyservicemaker import Probe, Receiver

from utils.pipeline.base import BaseRTSPPipeline, BaseVideoPipeline, validate_probe_interval
from utils.probe.presence_probe import DetPresenceVideoProbe, DetVisRTSPPresenceProbe
from utils.receiver.raw_capturer import RawCapturer
from utils.receiver.vis_capturer import VisCapturer


class PresenceRTSPPipeline(BaseRTSPPipeline):
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
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))
        self.capturer = capturer

    def build(self):
        self.attach_analyzer_probe(
            "presence",
            DetVisRTSPPresenceProbe(
                debouncer=self.debouncer,
                drawer=self.drawer,
                logger=self.logger,
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


class PresenceVideoPipeline(BaseVideoPipeline):
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
        self.messager = messager
        validate_probe_interval(self.pgie_interval, self.messager.get("interval", 0))
        validate_probe_interval(self.pgie_interval, self.logger.get("interval", 0))
        self.capturer = capturer

    def build(self):
        self.pipeline.attach(
            "analyzer",
            Probe(
                "presence",
                DetPresenceVideoProbe(
                    debouncer=self.debouncer,
                    drawer=self.drawer,
                    logger=self.logger,
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
