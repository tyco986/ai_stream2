import json
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path

LOG_FORMAT = "%(asctime)s %(message)s"


class DetLogger:
    def __init__(self, root, interval=0):
        self.root = Path(root)
        self.interval = int(interval)
        self.counter = 0
        assert self.interval >= 0, "interval must be greater than or equal to 0"
        self.logger = self.init_logger()

    def init_logger(self) -> logging.Logger:
        self.root.mkdir(parents=True, exist_ok=True)
        logger = logging.getLogger(f"det_logger.{self.root}")
        logger.setLevel(logging.INFO)
        logger.handlers.clear()
        logger.propagate = False
        handler = RotatingFileHandler(
            self.root / "det.log",
            maxBytes=1024 * 1024,
            backupCount=10,
            encoding="utf-8",
        )
        handler.setFormatter(logging.Formatter(LOG_FORMAT))
        logger.addHandler(handler)
        return logger

    def payload(self, results) -> list:
        payload = []
        for frame_result in results:
            class_num = {}
            for item in frame_result["objects"]:
                cls = int(item["cls"])
                class_num[cls] = class_num.get(cls, 0) + 1
            record = {
                "pad_index": int(frame_result["pad_index"]),
                "source_id": int(frame_result["source_id"]),
                "frame_number": int(frame_result["frame_number"]),
                "class_num": class_num,
            }
            payload.append(record)
        return payload

    def __call__(self, results) -> None:
        if self.interval == 0 or self.counter % self.interval == 0:
            payload = self.payload(results)
            self.logger.info("%s", json.dumps(payload, ensure_ascii=False))
            self.counter = 0
        self.counter += 1
