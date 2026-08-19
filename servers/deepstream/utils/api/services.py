from pathlib import Path

import yaml
from fastapi import UploadFile

from utils.api.constants import CONFIG_SAVE_DIR, LOG_ROOT
from utils.api.schemas import ApiEnvelope, AppError, StartPipelineRequest
from utils.manager.pipeline_manager import PipelineManager


class PipelineService:
    def save_config(self, filename: str, raw: bytes) -> None:
        name = Path(filename).name
        if not name or name in {".", ".."}:
            name = "pipeline.yaml"
        CONFIG_SAVE_DIR.mkdir(parents=True, exist_ok=True)
        path = CONFIG_SAVE_DIR / name
        path.write_bytes(raw)

    def start(self, upload: UploadFile) -> ApiEnvelope:
        raw = upload.file.read()
        self.save_config(upload.filename or "pipeline.yaml", raw)
        config = yaml.safe_load(raw)
        if not isinstance(config, dict):
            raise AppError("pipeline YAML must be a mapping")
        body = StartPipelineRequest.model_validate(config)
        if body.type not in PipelineManager.PIPELINES:
            raise AppError(f"unknown type {body.type!r}")
        if PipelineManager.is_running():
            raise AppError("pipeline is running")
        logger = dict(body.logger)
        logger["root"] = logger.get("root") or str(LOG_ROOT / body.name)
        PipelineManager.start(
            body.type,
            body.name,
            body.config_dir,
            logger,
            body.messager,
            body.drawer,
            body.debouncer,
            body.capturer,
        )
        result = ApiEnvelope.ok(data={"name": body.name, "type": body.type})
        return result

    def schema(self, pipeline_type: str) -> ApiEnvelope:
        if pipeline_type not in PipelineManager.PIPELINES:
            raise AppError(f"unknown type {pipeline_type!r}", status_code=404)
        result = ApiEnvelope.ok(data=PipelineManager().schema(pipeline_type))
        return result
