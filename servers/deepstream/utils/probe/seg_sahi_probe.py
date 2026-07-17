from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.seg_drawer import SegDrawer, SegFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger
from utils.probe.utils.messager.det_messager import DetMessager


class SegSahiVisRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = SegFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger(result)
            self.messager(result)


class SegSahiImageProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = SegDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger(result)
            self.messager(result)


class SegSahiVideoProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = SegDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        for result in self.drawer(batch_meta):
            self.logger(result)
            self.messager(result)
