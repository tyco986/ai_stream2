from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.det_fade_drawer import DetFadeDrawer
from utils.probe.utils.messager.det_messager import DetMessager


class DetSahiVideoProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = logger
        self.messager = DetMessager(**messager)
        self.drawer = DetFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
