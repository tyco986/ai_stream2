from pyservicemaker import BatchMetadataOperator

from utils.bridge.presence_capture_state import PresenceCaptureState
from utils.probe.det_vis_rtsp_probe import DetVisRTSPProbe
from utils.probe.utils.drawer.presence_drawer import PresenceFadeDrawer
from utils.probe.utils.messager.presence_messager import PresenceMessager


class DetVisRTSPPresenceProbe(DetVisRTSPProbe):
    def __init__(self, debouncer=dict(), logger=dict(), messager=dict(), drawer=dict()):
        BatchMetadataOperator.__init__(self)
        self.drawer = PresenceFadeDrawer(drawer=drawer, debouncer=debouncer)
        self.messager = PresenceMessager(**messager)
        self.logger = logger

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
            PresenceCaptureState.update(result)
