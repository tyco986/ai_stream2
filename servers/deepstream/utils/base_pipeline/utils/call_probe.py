from pyservicemaker import BatchMetadataOperator


class CallProbe(BatchMetadataOperator):
    def __init__(self, handler):
        self.handler = handler
        super().__init__()

    def handle_metadata(self, batch_meta):
        self.handler(batch_meta)
