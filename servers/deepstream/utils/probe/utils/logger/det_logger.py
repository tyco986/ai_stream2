import json
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path

LOG_FORMAT = "%(asctime)s %(message)s"
LOG_HEADER = (
    "# objects item format: [x1, y1, x2, y2, conf, cls_id, label]\n"
    "# line format: {timestamp} {json with pad_index, source_id, frame_number, class_num, objects}\n"
)


class DetLogFileHandler(RotatingFileHandler):
    def emit(self, record: logging.LogRecord) -> None:
        self.ensure_header()
        super().emit(record)

    def doRollover(self) -> None:
        super().doRollover()
        self.ensure_header()

    def ensure_header(self) -> None:
        path = Path(self.baseFilename)
        if not path.exists() or path.stat().st_size == 0:
            with path.open("w", encoding="utf-8") as handle:
                handle.write(LOG_HEADER)


class DetLogger:
    def __init__(self, root, interval=0):
        self.root = Path(root)
        self.interval = int(interval)
        self.runtime_interval = self.interval + 1 if self.interval > 0 else 0
        self.counter = 0
        assert self.interval >= 0, "interval must be greater than or equal to 0"
        self.init_logger()

    def init_logger(self) -> logging.Logger:
        self.root.mkdir(parents=True, exist_ok=True)
        self.logger = logging.getLogger(f"det_logger.{self.root}")
        self.logger.setLevel(logging.INFO)
        self.logger.handlers.clear()
        self.logger.propagate = False
        self.handler = DetLogFileHandler(
            self.root / "det.log",
            maxBytes=1024 * 1024,
            backupCount=10,
            encoding="utf-8",
        )
        self.handler.setFormatter(logging.Formatter(LOG_FORMAT))
        self.logger.addHandler(self.handler)

    def payload(self, result: dict) -> dict:
        class_num = {}
        objects = []
        for item in result["objects"]:
            cls = int(item["object"][5])
            class_num[cls] = class_num.get(cls, 0) + 1
            objects.append(item["object"])
        record = {
            "pad_index": int(result["pad_index"]),
            "source_id": int(result["source_id"]),
            "frame_number": int(result["frame_number"]),
            "class_num": class_num,
            "objects": objects,
        }
        return record

    def __call__(self, result: dict) -> None:
        if self.interval == 0 or self.counter % self.runtime_interval == 0:
            self.logger.info("%s", json.dumps(self.payload(result), ensure_ascii=False))
            self.counter = 0
        self.counter += 1
