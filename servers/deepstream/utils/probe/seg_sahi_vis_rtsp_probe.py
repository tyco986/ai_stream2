from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.messager.det_messager import DetMessager


class SegSahiVisRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = logger
        self.messager = DetMessager(**messager)
        self.drawer = drawer

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
