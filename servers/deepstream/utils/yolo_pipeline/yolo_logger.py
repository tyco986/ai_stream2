import json
import logging
import os

from pyservicemaker import BatchMetadataOperator

PIPELINE_LOGGER_NAME = "deepstream_api"
DEFAULT_INFERENCE_LOG_INTERVAL = int(os.environ.get("INFERENCE_LOG_INTERVAL", "30"))


def format_detection(obj) -> list:
    rect = obj.rect_params
    x1 = int(round(float(rect.left)))
    y1 = int(round(float(rect.top)))
    return [
        x1,
        y1,
        int(round(x1 + float(rect.width))),
        int(round(y1 + float(rect.height))),
        round(float(obj.confidence), 2),
        int(obj.class_id),
    ]


class YoloInferenceProbe(BatchMetadataOperator):
    def __init__(
        self,
        task: str,
        interval: int = DEFAULT_INFERENCE_LOG_INTERVAL,
        logger: logging.Logger | None = None,
    ) -> None:
        super().__init__()
        self.task = task
        self.interval = interval
        self.logger = logger or logging.getLogger(PIPELINE_LOGGER_NAME)
        self.frame_counts: dict[int, int] = {}

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            pad_index = int(frame_meta.pad_index)
            count = self.frame_counts.get(pad_index, 0) + 1
            self.frame_counts[pad_index] = count
            if count != 1 and (self.interval <= 0 or count % self.interval != 0):
                continue

            detections = [format_detection(obj) for obj in frame_meta.object_items]
            payload = {
                "task": self.task,
                "pad_index": pad_index,
                "frame_number": int(frame_meta.frame_number),
                "frame_count": count,
                "num_detections": len(detections),
                "detections": detections,
            }
            self.logger.info("inference %s", json.dumps(payload, ensure_ascii=False))
            for handler in self.logger.handlers:
                handler.flush()
        return True
