import shutil
from pathlib import Path

import yaml
from fastapi import UploadFile

from utils.api.schemas import ApiEnvelope, AppError
from utils.manager.generator_manager import GeneratorManager


class GeneratorService:
    def generate(self, upload: UploadFile) -> ApiEnvelope:
        config = yaml.safe_load(upload.file.read())
        if not isinstance(config, dict):
            raise AppError("generator YAML must be a mapping")
        if "config_save_dir" not in config:
            raise AppError("missing config_save_dir")
        if "generator" not in config:
            raise AppError("missing generator")
        payload = dict(config)
        config_save_dir = Path(payload.pop("config_save_dir"))
        generator_name = payload.pop("generator")
        if "pipeline_name" not in payload:
            payload["pipeline_name"] = config_save_dir.name
        if generator_name not in GeneratorManager.GENERATORS:
            raise AppError(f"unknown type {generator_name!r}")
        if config_save_dir.exists():
            if not config_save_dir.is_dir():
                raise AppError(f"config_save_dir is not a directory: {config_save_dir}")
            shutil.rmtree(config_save_dir)
        config_save_dir.mkdir(parents=True, exist_ok=True)
        GeneratorManager().generate(generator_name, config_save_dir, **payload)
        result = ApiEnvelope.ok(data=str(config_save_dir))
        return result

    def schema(self, generator_name: str) -> ApiEnvelope:
        if generator_name not in GeneratorManager.GENERATORS:
            raise AppError(f"unknown type {generator_name!r}", status_code=404)
        result = ApiEnvelope.ok(data=GeneratorManager().schema(generator_name))
        return result
