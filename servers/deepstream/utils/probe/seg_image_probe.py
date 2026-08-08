from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.seg_drawer import SegDrawer
from utils.probe.utils.messager.det_messager import DetMessager


class SegImageProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = logger
        self.messager = DetMessager(**messager)
        self.drawer = SegDrawer(**drawer)

    def handle_metadata(self, batch_meta) -> None:
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
