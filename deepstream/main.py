#!/usr/bin/env python3
"""Build and run a DeepStream pipeline from YAML (pyservicemaker)."""

import argparse
import logging
import signal
import sys
from multiprocessing import Process
from pathlib import Path

from pyservicemaker import Pipeline, Probe

from utils.logging_config import (
    DEFAULT_DETECTION_LOG_INTERVAL,
    RollingLoggingConfigurator,
    YoloDetectionLogger,
)

ROOT = Path(__file__).resolve().parent
DEFAULT_CONFIG = ROOT / "configs" / "pipeline.yml"
PIPELINE_NAME = "yolo26-detection"


def _build_pipeline(config: Path) -> Pipeline:
    pipeline = Pipeline(PIPELINE_NAME, str(config.resolve()))
    pipeline.attach(
        "pgie",
        Probe(
            "yolo_detection_log",
            YoloDetectionLogger(interval=DEFAULT_DETECTION_LOG_INTERVAL),
        ),
    )
    return pipeline


class PipelineRunner:
    """Child: run GStreamer; release RTSP on SIGTERM from supervisor."""

    def __init__(self, pipeline: Pipeline, logger: logging.Logger) -> None:
        self._pipeline = pipeline
        self._logger = logger
        self._stopped = False

    def run(self) -> None:
        signal.signal(signal.SIGTERM, self._on_sigterm)
        self._logger.info("Starting pipeline")
        self._pipeline.start()
        try:
            self._pipeline.wait()
        finally:
            self._shutdown()

    def _on_sigterm(self, signum: int, _frame) -> None:
        self._logger.warning("Stopping pipeline")
        self._shutdown()
        raise SystemExit(128 + signum)

    def _shutdown(self) -> None:
        if self._stopped:
            return
        self._stopped = True
        self._pipeline.stop()


class PipelineSupervisor:
    """
    Parent owns the OS process; child owns GStreamer.

    pipeline.wait() blocks in native code, so Ctrl+C must be handled here:
    one SIGINT terminates the child, which runs stop/deactivate on SIGTERM.
    """

    _GRACEFUL_WAIT_SEC = 3

    def __init__(self, config: Path) -> None:
        self._config = config.resolve()
        self._process: Process | None = None

    def run(self) -> None:
        self._process = Process(target=_child_main, args=(self._config,))
        self._process.start()
        try:
            self._process.join()
        except KeyboardInterrupt:
            self._terminate_child()

    def _terminate_child(self) -> None:
        proc = self._process
        if proc is None or not proc.is_alive():
            return
        print("\nStopping pipeline...", file=sys.stderr, flush=True)
        proc.terminate()
        try:
            proc.join(self._GRACEFUL_WAIT_SEC)
        except KeyboardInterrupt:
            pass
        if proc.is_alive():
            proc.kill()
            proc.join()
        print("Stopped.", file=sys.stderr, flush=True)


def _child_main(config: Path) -> None:
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    logger = RollingLoggingConfigurator().configure()
    logger.info("Loading pipeline config: %s", config)
    pipeline = _build_pipeline(config)
    logger.info("Pipeline ready")
    PipelineRunner(pipeline, logger).run()


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Run DeepStream pipeline from YAML.")
    parser.add_argument("-c", "--config", type=Path, default=DEFAULT_CONFIG)
    args = parser.parse_args(argv)
    config = args.config if args.config.is_absolute() else ROOT / args.config
    PipelineSupervisor(config).run()


if __name__ == "__main__":
    main()
