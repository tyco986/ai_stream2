from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.pose2d_drawer import Pose2DDrawer
from utils.probe.utils.messager.det_messager import DetMessager


class Pose2DRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = logger
        self.messager = DetMessager(**messager)
        self.drawer = Pose2DDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger.log_detection(result)
            self.messager(result)
