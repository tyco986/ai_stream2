from pyservicemaker import BatchMetadataOperator


class TimesProbe(BatchMetadataOperator):
    def __init__(self, logger):
        self.logger = logger
        super().__init__()

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            self.logger.log_times(
                {
                    "pad_index": int(frame_meta.pad_index),
                    "source_id": int(frame_meta.source_id),
                    "frame_number": int(frame_meta.frame_number),
                }
            )
