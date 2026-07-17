import json
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path

from utils.probe.utils.logger.det_logger import DetLogger, LOG_FORMAT


class PresenceLogger(DetLogger):
    def __call__(self, result: dict) -> None:
        if self.interval == 0 or self.counter % self.runtime_interval == 0:
            self.logger.info("%s", json.dumps(self.payload(result), ensure_ascii=False))
            self.logger.info("%s", json.dumps(result["event"], ensure_ascii=False))
            self.counter = 0
        self.counter += 1