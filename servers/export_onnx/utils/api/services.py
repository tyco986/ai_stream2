import shutil
from pathlib import Path

import yaml
from fastapi import UploadFile

from utils.api.constants import DEFAULT_MODEL_ROOT
from utils.api.schemas import ApiEnvelope, AppError, OnnxExportConfig
from utils.manager.onnx_exporter_manager import OnnxExporterManager


class ExportOnnxService:
    def parse_config(self, upload: UploadFile) -> OnnxExportConfig:
        payload = yaml.safe_load(upload.file.read())
        if not isinstance(payload, dict):
            raise AppError("config YAML must be a mapping")
        config = OnnxExportConfig.model_validate(payload)
        return config

    def save_weights(self, upload: UploadFile) -> Path:
        filename = Path(upload.filename or "model.pt").name
        if Path(filename).suffix.lower() != ".pt":
            raise AppError(f"input must be a .pt file: {filename}")
        weights = DEFAULT_MODEL_ROOT / "pt" / filename
        weights.parent.mkdir(parents=True, exist_ok=True)
        weights.unlink(missing_ok=True)
        with weights.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        return weights

    def export(self, input: UploadFile, config: UploadFile) -> ApiEnvelope:
        cfg = self.parse_config(config)
        if cfg.type not in OnnxExporterManager.EXPORTERS:
            raise AppError(f"unknown type {cfg.type!r}")
        weights = self.save_weights(input)
        output_dir = DEFAULT_MODEL_ROOT / "onnx" / weights.stem
        if output_dir.exists():
            shutil.rmtree(output_dir)
        output_dir.mkdir(parents=True)
        OnnxExporterManager().export(
            cfg.type,
            weights,
            cfg.size,
            cfg.opset,
            cfg.batch,
            cfg.dynamic,
            cfg.simplify,
            cfg.max_det,
            cfg.conf,
            output_dir,
            cfg.iou,
        )
        payload = ApiEnvelope.ok(data=str(output_dir))
        return payload
