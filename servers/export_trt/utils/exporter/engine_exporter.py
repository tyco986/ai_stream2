import json
import logging
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

from utils.api.constants import (
    DEFAULT_PRECISION,
    LABELS_NAME,
    LOGGER_NAME,
    META_JSON_NAME,
    MODEL_ROOT,
)
from utils.exporter.common import (
    AppError,
    ExportEngineResult,
    OnnxBundle,
    run_export_cli,
    validate_precision,
)


class EngineExporter:
    def __init__(self, model_root: Path = MODEL_ROOT) -> None:
        self.model_root = model_root
        self.trt_root = model_root / "trt"
        self.logger = logging.getLogger(LOGGER_NAME)

    def resolve_input(self, input_path: str) -> Path:
        path = Path(input_path).expanduser()
        if not path.is_absolute():
            path = self.model_root / path
        path = path.resolve()
        if not path.is_dir():
            raise AppError(f"input not found or not a directory: {path}")
        return path

    def run_trtexec(
        self,
        bundle: OnnxBundle,
        engine_path: Path,
        batch_size: int,
        gpu_id: int,
        precision: str,
        opt_level: int | None = None,
    ) -> list[str]:
        command = bundle.build_trtexec_command(
            engine_path, batch_size, gpu_id, precision, opt_level
        )
        self.logger.info("trtexec start cmd=%s", command)
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            raise AppError((result.stderr or result.stdout or "trtexec failed").strip())
        if not engine_path.is_file():
            raise AppError(f"trtexec did not produce engine: {engine_path}")
        self.logger.info("trtexec done engine=%s", engine_path)
        return command

    def export_engine(
        self,
        input_path: str,
        batch_size: int | None,
        gpu_id: int,
        precision: str = DEFAULT_PRECISION,
        opt_level: int | None = None,
    ) -> ExportEngineResult:
        precision = validate_precision(precision)
        bundle = OnnxBundle.load(self.resolve_input(input_path))
        resolved_batch = bundle.resolve_batch(batch_size)

        bundle_dir = self.trt_root / bundle.folder.name
        if bundle_dir.exists():
            shutil.rmtree(bundle_dir)
        bundle_dir.mkdir(parents=True)

        shutil.copy2(bundle.labels_path, bundle_dir / LABELS_NAME)
        engine_path = bundle_dir / f"{bundle.stem}.engine"
        command = self.run_trtexec(
            bundle, engine_path, resolved_batch, gpu_id, precision, opt_level
        )
        build_time = datetime.now(timezone.utc).isoformat()
        (bundle_dir / META_JSON_NAME).write_text(
            json.dumps(
                bundle.build_output_meta(
                    resolved_batch,
                    gpu_id,
                    precision,
                    build_time,
                    opt_level,
                ),
                ensure_ascii=False,
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )

        self.logger.info("export done stem=%s bundle=%s", bundle.stem, bundle_dir)
        return ExportEngineResult(bundle_dir=bundle_dir, command=command)


if __name__ == "__main__":
    run_export_cli(
        EngineExporter(),
        "Export TensorRT engine from ONNX bundle directory",
    )
