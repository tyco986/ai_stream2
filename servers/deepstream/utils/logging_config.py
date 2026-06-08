"""Rolling file logging setup for DeepStream."""

import logging
import sys
from logging.handlers import RotatingFileHandler
from pathlib import Path

from pyservicemaker import BatchMetadataOperator

_config_file = Path(__file__).resolve()

LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
DEFAULT_DETECTION_LOG_INTERVAL = 1500
DEEPSTREAM_ROOT = _config_file.parent.parent
PROJECT_ROOT = (
    _config_file.parents[3] if len(_config_file.parents) > 3 else DEEPSTREAM_ROOT
)
DEFAULT_LOG_DIR = PROJECT_ROOT / "logs" / "deepstream"
DEFAULT_MAX_BYTES = 10 * 1024 * 1024
DEFAULT_BACKUP_COUNT = 10
DEFAULT_LOG_FILENAME = "deepstream.log"


class ServiceLogger:
    """Configure a named logger with rotating file and stderr handlers."""

    def __init__(
        self,
        log_dir: Path | None = None,
        name: str = "deepstream",
        level: int = logging.INFO,
        max_bytes: int = DEFAULT_MAX_BYTES,
        backup_count: int = DEFAULT_BACKUP_COUNT,
        console: bool = True,
        log_filename: str = DEFAULT_LOG_FILENAME,
    ) -> None:
        self._log_dir = log_dir or DEFAULT_LOG_DIR
        self._name = name
        self._level = level
        self._max_bytes = max_bytes
        self._backup_count = backup_count
        self._console = console
        self._log_filename = log_filename

    def configure(self) -> logging.Logger:
        self._log_dir.mkdir(parents=True, exist_ok=True)
        logger = logging.getLogger(self._name)
        logger.setLevel(self._level)
        logger.handlers.clear()
        logger.propagate = False

        formatter = logging.Formatter(LOG_FORMAT)
        file_handler = RotatingFileHandler(
            self._log_dir / self._log_filename,
            maxBytes=self._max_bytes,
            backupCount=self._backup_count,
            encoding="utf-8",
        )
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)

        if self._console:
            console_handler = logging.StreamHandler(sys.stderr)
            console_handler.setFormatter(formatter)
            logger.addHandler(console_handler)

        return logger


class YoloDetectionLogger(BatchMetadataOperator):
    """Log YOLO detections every N frames; frame 0 is always included."""

    def __init__(self, interval: int = DEFAULT_DETECTION_LOG_INTERVAL) -> None:
        super().__init__()
        self._logger = logging.getLogger("deepstream")
        self._interval = interval

    def handle_metadata(self, batch_meta) -> None:
        for frame_meta in batch_meta.frame_items:
            if frame_meta.frame_number % self._interval != 0:
                continue
            objects = [
                {
                    "class_id": obj.class_id,
                    "label": obj.label,
                    "confidence": round(obj.confidence, 3),
                    "bbox": (
                        round(obj.rect_params.left, 1),
                        round(obj.rect_params.top, 1),
                        round(obj.rect_params.width, 1),
                        round(obj.rect_params.height, 1),
                    ),
                }
                for obj in frame_meta.object_items
            ]
            self._logger.info(
                "frame=%s count=%s objects=%s",
                frame_meta.frame_number,
                len(objects),
                objects,
            )
