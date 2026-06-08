import logging
import signal
import sys
from multiprocessing import Process
from pathlib import Path

from pyservicemaker import Pipeline

from utils.logging_config import ServiceLogger

PIPELINE_NAME = "pipeline"
_GRACEFUL_WAIT_SEC = 3


class _PipelineRunner:
    """Child process: load YAML (or use injected Pipeline), start/wait/stop."""

    def __init__(
        self,
        config: Path,
        log_root: Path,
        pipeline: Pipeline | None = None,
    ) -> None:
        self._config = config.resolve()
        self._log_root = log_root.resolve()
        self._pipeline = pipeline
        self._logger: logging.Logger | None = None
        self._stopped = False

    def run(self) -> None:
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        self._logger = ServiceLogger(log_dir=self._log_root).configure()
        if self._pipeline is None:
            self._logger.info("Loading pipeline config: %s", self._config)
            self._pipeline = Pipeline(PIPELINE_NAME, str(self._config))
        self._logger.info("Pipeline ready")
        signal.signal(signal.SIGTERM, self._on_sigterm)
        self._logger.info("Starting pipeline")
        self._pipeline.start()
        try:
            self._pipeline.wait()
        finally:
            self._shutdown()

    def _on_sigterm(self, signum: int, _frame) -> None:
        if self._logger is not None:
            self._logger.warning("Stopping pipeline")
        self._shutdown()
        raise SystemExit(128 + signum)

    def _shutdown(self) -> None:
        if self._stopped or self._pipeline is None:
            return
        self._stopped = True
        self._pipeline.stop()


class PipelineRunner:
    """
    Parent process: owns _PipelineRunner, spawn child, handle Ctrl+C.

    pipeline.wait() blocks in native code; SIGINT is handled here.
    """

    def __init__(
        self,
        config: Path,
        log_root: Path,
        pipeline: Pipeline | None = None,
    ) -> None:
        self._runner = _PipelineRunner(config, log_root, pipeline)
        self._process: Process | None = None

    def run(self) -> None:
        self._process = Process(target=self._runner.run)
        self._process.start()
        try:
            self._process.join()
        except KeyboardInterrupt:
            self._stop_child()

    def _stop_child(self) -> None:
        proc = self._process
        if proc is None or not proc.is_alive():
            return
        print("\nStopping pipeline...", file=sys.stderr, flush=True)
        proc.terminate()
        try:
            proc.join(_GRACEFUL_WAIT_SEC)
        except KeyboardInterrupt:
            pass
        if proc.is_alive():
            proc.kill()
            proc.join()
        print("Stopped.", file=sys.stderr, flush=True)
