from pyservicemaker import BatchMetadataOperator


class DetFadeCacheProbe(BatchMetadataOperator):
    def __init__(self, drawer):
        self.drawer = drawer
        super().__init__()

    def handle_metadata(self, batch_meta):
        self.drawer.cache_detections(batch_meta)
