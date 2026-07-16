from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.drawer.pose_drawer import PoseDrawer, PoseFadeDrawer
from utils.probe.utils.logger.det_logger import DetLogger
from utils.probe.utils.messager.det_messager import DetMessager
from utils.probe.utils.parser.pose_parser import PoseBatchMetaParser, PoseKptsCache


class PoseKptsCacheProbe(BatchMetadataOperator):
    """Read pose mask_params before demux; after demux the mask wrapper is dangling."""

    def __init__(self, kpts_cache: PoseKptsCache):
        super().__init__()
        self.kpts_cache = kpts_cache

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            kpts_list = [
                PoseBatchMetaParser.parse_kpts(obj.mask_params)
                for obj in frame_meta.object_items
            ]
            self.kpts_cache.put_frame(frame_meta.source_id, frame_meta.frame_number, kpts_list)


class PoseRTSPProbe(BatchMetadataOperator):
    def __init__(self, drawer=dict(), logger=dict(), messager=dict(), kpts_cache=None):
        super().__init__()
        self.logger = DetLogger(**logger)
        self.messager = DetMessager(**messager)
        self.drawer = PoseFadeDrawer(**drawer)
        self.kpts_cache = kpts_cache if kpts_cache is not None else PoseKptsCache()

    def handle_metadata(self, batch_meta):
        frame_meta = next(iter(batch_meta.frame_items))
        kpts_list = self.kpts_cache.pop_frame(frame_meta.source_id, frame_meta.frame_number)
        parser = PoseBatchMetaParser(batch_meta, kpts_list=kpts_list)
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
