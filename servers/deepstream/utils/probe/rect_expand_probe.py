from pyservicemaker import BatchMetadataOperator

from utils.probe.utils.preprocessor.rect_expander import RectExpander


class RectExpandProbe(BatchMetadataOperator):
    def __init__(self, infer_height=256, infer_width=192, padding=1.25):
        super().__init__()
        self.expander = RectExpander(
            infer_height=infer_height,
            infer_width=infer_width,
            padding=padding,
        )

    def handle_metadata(self, batch_meta):
        self.expander(batch_meta)
