from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.det_drawer import DetDrawer, DetFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger
from utils.probe.utils.messager.det_messager import DetMessager
from utils.probe.utils.parser.det_parser import DetBatchMetaParser


class DetRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.logger(result)
        self.messager(result)


class DetImageProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.logger(result)
        self.messager(result)


class DetVideoProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.logger(result)
        self.messager(result)
