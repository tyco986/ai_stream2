#!/usr/bin/env python3
"""Build and run a DeepStream pipeline from YAML (pyservicemaker)."""

import argparse
import logging
import signal
import sys
from pathlib import Path
from typing import Any

import yaml
from pyservicemaker import Pipeline, Probe

from utils.logging_config import InferenceFrameLogger, RollingLoggingConfigurator

ROOT = Path(__file__).resolve().parent
DEFAULT_CONFIG = ROOT / "configs" / "pipeline.yml"


class PipelineYamlBuilder:
    def __init__(self, config_path: Path):
        path = config_path.resolve()
        with path.open(encoding="utf-8") as handle:
            data = yaml.safe_load(handle)
        if not isinstance(data, dict) or "pipeline" not in data:
            raise ValueError(f"Invalid pipeline config (missing 'pipeline'): {path}")
        self._app = data.get("app") or {}
        self._pipeline = data["pipeline"]

    def build(self) -> Pipeline:
        pipeline = Pipeline(self._pipeline["name"])
        elements = list(self._pipeline.get("elements", []))
        sink = self._pipeline.get("sink")
        if sink:
            elements.append(
                {"name": "sink", "type": sink["type"], "properties": sink.get("properties")}
            )
        for element in elements:
            raw_props = element.get("properties")
            props = self._resolve(raw_props, self._app) if raw_props else None
            pipeline.add(element["type"], element["name"], props)
        for group in self._pipeline.get("links", []):
            pipeline.link(
                *(tuple(item) if isinstance(item, list) else item for item in group)
            )
        return pipeline

    def _resolve(self, value: Any, context: dict[str, Any]) -> Any:
        if isinstance(value, str) and value.startswith("${") and value.endswith("}"):
            return context.get(value[2:-1], value)
        if isinstance(value, dict):
            return {k: self._resolve(v, context) for k, v in value.items()}
        if isinstance(value, list):
            return [self._resolve(item, context) for item in value]
        return value


class PipelineRunner:
    """Run pipeline; SIGINT/SIGTERM call stop() to unblock wait()."""

    def __init__(self, pipeline: Pipeline, logger: logging.Logger) -> None:
        self._pipeline = pipeline
        self._logger = logger
        self._stopping = False
        self._pipeline.attach("pgie", Probe("infer_log", InferenceFrameLogger()))

    def run(self) -> None:
        signal.signal(signal.SIGINT, self._on_signal)
        signal.signal(signal.SIGTERM, self._on_signal)
        self._logger.info("Starting pipeline")
        self._pipeline.start()
        try:
            self._pipeline.wait()
        except KeyboardInterrupt:
            self._request_stop(signal.SIGINT)

    def _on_signal(self, signum: int, _frame) -> None:
        self._request_stop(signum)

    def _request_stop(self, signum: int) -> None:
        if self._stopping:
            raise SystemExit(128 + signum)
        self._stopping = True
        self._logger.warning("Signal %s received, stopping pipeline", signum)
        self._pipeline.stop()


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Run DeepStream pipeline from YAML.")
    parser.add_argument("-c", "--config", type=Path, default=DEFAULT_CONFIG)
    args = parser.parse_args(argv)
    logger = RollingLoggingConfigurator().configure()
    config = args.config if args.config.is_absolute() else ROOT / args.config
    logger.info("Loading pipeline config: %s", config)
    pipeline = PipelineYamlBuilder(config).build()
    logger.info("Pipeline ready")
    PipelineRunner(pipeline, logger).run()


if __name__ == "__main__":
    main()
