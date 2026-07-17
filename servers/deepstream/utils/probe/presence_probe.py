from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.presence_drawer import PresenceFadeDrawer
from utils.probe.utils.logger.presence_logger import PresenceLogger
from utils.probe.utils.messager.det_messager import DetMessager
from utils.bridge.presence_capture_state import PresenceCaptureState

from .det_probe import DetVisRTSPProbe, DetVideoProbe


class DetVisRTSPPresenceProbe(DetVisRTSPProbe):
    def __init__(self, debouncer=dict(), logger=dict(), messager=dict(), drawer=dict()):
        BatchMetadataOperator.__init__(self)
        self.drawer = PresenceFadeDrawer(drawer=drawer, debouncer=debouncer)
        self.messager = DetMessager(**messager)
        self.logger = PresenceLogger(**logger)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger(result)
            self.messager(result)
            event = result.get("event", {"event_code": ""})
            PresenceCaptureState.update(
                result["pad_index"],
                result["frame_number"],
                event.get("event_code", ""),
            )


class DetPresenceVideoProbe(DetVideoProbe):
    def __init__(self, debouncer=dict(), logger=dict(), messager=dict(), drawer=dict()):
        BatchMetadataOperator.__init__(self)
        self.drawer = PresenceFadeDrawer(drawer=drawer, debouncer=debouncer)
        self.messager = DetMessager(**messager)
        self.logger = PresenceLogger(**logger)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger(result)
            self.messager(result)
            event = result.get("event", {"event_code": ""})
            PresenceCaptureState.update(
                result["pad_index"],
                result["frame_number"],
                event.get("event_code", ""),
            )
