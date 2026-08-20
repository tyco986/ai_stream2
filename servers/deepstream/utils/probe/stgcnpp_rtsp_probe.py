from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.stgcnpp_drawer import StgcnppDrawer
from utils.probe.utils.messager.det_messager import DetMessager


class StgcnppRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = logger
        self.messager = DetMessager(**messager)
        self.drawer = StgcnppDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
