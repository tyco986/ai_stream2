from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.pose_drawer import PoseDrawer, PoseFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger
from utils.probe.utils.messager.det_messager import DetMessager
from utils.probe.utils.parser.pose_parser import PoseBatchMetaParser


class PoseRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = PoseBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.logger(result)
        self.messager(result)


class PoseImageProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = PoseBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.logger(result)
        self.messager(result)


class PoseVideoProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = PoseBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.logger(result)
        self.messager(result)
