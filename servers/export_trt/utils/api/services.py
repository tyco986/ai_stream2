import shlex
from pathlib import Path

from utils.api.constants import DEFAULT_PRECISION, MODEL_ROOT
from utils.api.schemas import ApiEnvelope
from utils.exporter import AppError, EngineExporter

__all__ = ["AppError", "ExportEngineService"]


class ExportEngineService:
    def __init__(self, model_root: Path = MODEL_ROOT) -> None:
        self.model_root = model_root
        self.exporter = EngineExporter(model_root)

    def export_engine(
        self,
        input_path: str,
        batch_size: int | None,
        gpu_id: int,
        precision: str = DEFAULT_PRECISION,
        opt_level: int | None = None,
    ) -> ApiEnvelope:
        result = self.exporter.export_engine(
            input_path,
            batch_size,
            gpu_id,
            precision,
            opt_level,
        )
        return ApiEnvelope.ok(
            data=str(result.bundle_dir),
            command=shlex.join(result.command),
        )
