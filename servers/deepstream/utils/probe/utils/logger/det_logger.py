import json
import logging
from pathlib import Path

from utils.probe.utils.timer import Timer

LOG_FORMAT = "%(asctime)s %(message)s"
LOG_HEADER = (
    "# objects item format: [x1, y1, x2, y2, conf, cls_id, label, id]\n"
    "# detection line: {timestamp} {json with pad_index, source_id, frame_number, class_num, objects}\n"
    "# times line: {timestamp} {json times}\n"
)


class ProbeLogFileHandler(logging.FileHandler):
    def __init__(self, filename, max_bytes=1024 * 1024, encoding="utf-8"):
        self.max_bytes = max_bytes
        super().__init__(filename, encoding=encoding)

    def emit(self, record: logging.LogRecord) -> None:
        self.ensure_header()
        if self.should_rollover(record):
            self.do_rollover()
        super().emit(record)

    def should_rollover(self, record: logging.LogRecord) -> bool:
        exceed = False
        if self.max_bytes > 0:
            if self.stream is None:
                self.stream = self._open()
            msg = f"{self.format(record)}\n"
            self.stream.seek(0, 2)
            exceed = self.stream.tell() + len(msg.encode(self.encoding or "utf-8")) >= self.max_bytes
        return exceed

    def do_rollover(self) -> None:
        if self.stream:
            self.stream.close()
            self.stream = None
        path = Path(self.baseFilename)
        with path.open("w", encoding="utf-8") as handle:
            handle.write(LOG_HEADER)
        self.stream = self._open()

    def ensure_header(self) -> None:
        path = Path(self.baseFilename)
        if not path.exists() or path.stat().st_size == 0:
            with path.open("w", encoding="utf-8") as handle:
                handle.write(LOG_HEADER)


class DetLogger:
    def __init__(self, root, interval=0, times=None):
        self.root = Path(root)
        self.interval = int(interval)
        self.times = times
        self.timer = Timer() if times is None else Timer(elements=times)
        self.runtime_interval = self.interval if self.interval > 0 else 0
        self.counters = {}
        self.loggers = {}
        self.pending_times = set()
        assert self.interval >= 0, "interval must be greater than or equal to 0"
        self.root.mkdir(parents=True, exist_ok=True)

    def get_logger(self, pad_index: int) -> logging.Logger:
        logger = self.loggers.get(pad_index)
        if logger is None:
            logger = logging.getLogger(f"probe_logger.{self.root}.{pad_index}")
            logger.setLevel(logging.INFO)
            logger.handlers.clear()
            logger.propagate = False
            handler = ProbeLogFileHandler(
                self.root / f"probe_{pad_index}.log",
                max_bytes=1024 * 1024,
                encoding="utf-8",
            )
            handler.setFormatter(logging.Formatter(LOG_FORMAT))
            logger.addHandler(handler)
            self.loggers[pad_index] = logger
        return logger

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

    def times_payload(self, result: dict) -> dict:
        record = self.timer.read(
            int(result["source_id"]),
            int(result["frame_number"]),
        )
        return record

    def times_key(self, result: dict) -> tuple:
        key = (
            int(result["pad_index"]),
            int(result["source_id"]),
            int(result["frame_number"]),
        )
        return key

    def log_detection(self, result: dict) -> None:
        pad_index = int(result["pad_index"])
        counter = self.counters.get(pad_index, 0)
        if self.interval == 0 or counter % self.runtime_interval == 0:
            logger = self.get_logger(pad_index)
            logger.info("%s", json.dumps(self.payload(result), ensure_ascii=False))
            self.pending_times.add(self.times_key(result))
            counter = 0
        self.counters[pad_index] = counter + 1

    def log_times(self, result: dict) -> None:
        key = self.times_key(result)
        if key in self.pending_times:
            logger = self.get_logger(int(result["pad_index"]))
            logger.info("%s", json.dumps(self.times_payload(result), ensure_ascii=False))
            self.pending_times.discard(key)

    def __call__(self, result: dict) -> None:
        self.log_detection(result)
        self.log_times(result)
