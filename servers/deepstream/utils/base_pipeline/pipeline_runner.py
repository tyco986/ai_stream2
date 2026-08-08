import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path

LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"


class PipelineRunner:
    """Start and block until a pyservicemaker pipeline finishes."""

    def __init__(self, pipeline, logger=dict()):
        self.pipeline = pipeline
        self.root = Path(logger["root"])
        self.logger = self.init_logger()

    def init_logger(self) -> logging.Logger:
        self.root.mkdir(parents=True, exist_ok=True)
        logger = logging.getLogger(f"pipeline_runner.{self.root}")
        logger.setLevel(logging.INFO)
        logger.handlers.clear()
        logger.propagate = False
        handler = RotatingFileHandler(
            self.root / "runner.log",
            maxBytes=1024 * 1024,
            backupCount=10,
            encoding="utf-8",
        )
        handler.setFormatter(logging.Formatter(LOG_FORMAT))
        logger.addHandler(handler)
        return logger

    def start(self):
        self.logger.info("pipeline start")
        self.pipeline.start().wait()
        self.logger.info("pipeline finished")
