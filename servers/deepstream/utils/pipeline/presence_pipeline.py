from pyservicemaker import Receiver

from utils.pipeline.base import BaseRTSPPipeline
from utils.probe.presence_probe import PresenceProbe
from utils.receiver.raw_capturer import RawCapturer
from utils.receiver.vis_capturer import VisCapturer


class PresenceRTSPPipeline(BaseRTSPPipeline):
    def __init__(
        self,
        config_dir,
        pipeline_name,
        debouncer=dict(),
        logger=dict(),
        messager=dict(),
        capturer=dict(),
    ):
        super().__init__(config_dir, pipeline_name)
        self.debouncer = debouncer
        self.logger = logger
        self.messager = messager
        self.capturer = capturer

    def build(self):
        self.attach_branch_probes(
            "presence",
            lambda index: PresenceProbe(
                debouncer=self.debouncer,
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
