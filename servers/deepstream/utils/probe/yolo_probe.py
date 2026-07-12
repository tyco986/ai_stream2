from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.det_drawer import DetDrawer, DetFadeDrawer
from utils.probe.utils.drawer.pose_drawer import PoseDrawer, PoseFadeDrawer
from utils.probe.utils.drawer.seg_drawer import SegDrawer, SegFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger
from utils.probe.utils.messager.det_messager import DetMessager
from utils.probe.utils.parser.det_parser import DetBatchMetaParser
from utils.probe.utils.parser.pose_parser import PoseBatchMetaParser
from utils.probe.utils.parser.seg_parser import SegBatchMetaParser


class DetRTSPProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetDrawer()

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class SegRTSPProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = SegFadeDrawer()

    def handle_metadata(self, batch_meta):
        parser = SegBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class PoseRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = PoseBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class DetImageProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetDrawer()

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class SegImageProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = SegDrawer()

    def handle_metadata(self, batch_meta):
        parser = SegBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class PoseImageProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = PoseBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class DetVideoProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetFadeDrawer()

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class SegVideoProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = SegFadeDrawer()

    def handle_metadata(self, batch_meta):
        parser = SegBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class PoseVideoProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseFadeDrawer(**drawer)

    def handle_metadata(self, batch_meta):
        parser = PoseBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class DetSahiRTSPProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetDrawer()

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class DetSahiImageProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetDrawer()

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)


class DetSahiVideoProbe(BatchMetadataOperator):
    def __init__(self, logger=dict(), messager=dict()):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = DetFadeDrawer()

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.logger(results)
        self.messager(results)
