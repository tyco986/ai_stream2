#!/usr/bin/env python3
"""Build and run a DeepStream pipeline from YAML (pyservicemaker)."""

import argparse
from pathlib import Path

from utils.logging_config import DEFAULT_LOG_DIR
from utils.pipeline_runner import PipelineRunner

_main_file = Path(__file__).resolve()
DEEPSTREAM_ROOT = _main_file.parent
PROJECT_ROOT = (
    _main_file.parents[2] if len(_main_file.parents) > 2 else DEEPSTREAM_ROOT
)
DEFAULT_CONFIG = PROJECT_ROOT / "configs" / "deepstream" / "pipeline.yml"


def _resolve_config(path: Path) -> Path:
    return path if path.is_absolute() else PROJECT_ROOT / path


def _resolve_log_root(path: Path) -> Path:
    return path if path.is_absolute() else PROJECT_ROOT / path


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Run DeepStream pipeline from YAML.")
    parser.add_argument(
        "-c",
        "--config",
        type=Path,
        default=DEFAULT_CONFIG,
        help="Pipeline YAML (default: configs/deepstream/pipeline.yml)",
    )
    parser.add_argument(
        "--log-root",
        type=Path,
        default=DEFAULT_LOG_DIR,
        help="Log directory (default: logs/deepstream under project root)",
    )
    args = parser.parse_args(argv)
    PipelineRunner(
        _resolve_config(args.config),
        _resolve_log_root(args.log_root),
    ).run()


if __name__ == "__main__":
    main()
