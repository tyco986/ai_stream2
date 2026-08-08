import logging
import shutil
from pathlib import Path

import yaml
from fastapi import UploadFile

from utils.api.constants import LOGGER_NAME
from utils.api.schemas import ApiEnvelope
from utils.registry import GeneratorRegistry


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class GenerateService:
    def __init__(self, registry: GeneratorRegistry) -> None:
        self.registry = registry
        self.logger = logging.getLogger(LOGGER_NAME)

    def list_types(self) -> ApiEnvelope:
        items = [{"generator": name} for name in self.registry.names()]
        return ApiEnvelope.ok(data={"items": items})

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
        if not self.registry.contains(generator_name):
            raise AppError(
                f"unsupported generator: {generator_name!r} "
                f"(supported: {', '.join(self.registry.names())})"
            )

        generator_cls = self.registry.resolve(generator_name)
        generator = generator_cls(**payload)
        if config_save_dir.exists():
            if not config_save_dir.is_dir():
                raise AppError(f"config_save_dir is not a directory: {config_save_dir}")
            shutil.rmtree(config_save_dir)
        config_save_dir.mkdir(parents=True, exist_ok=True)
        generator.write(config_save_dir)

        self.logger.info(
            "generate done generator=%s config_save_dir=%s",
            generator_name,
            config_save_dir,
        )
        return ApiEnvelope.ok(data=str(config_save_dir))


class SchemaService:
    SCHEMA_DIR = Path(__file__).resolve().parents[2] / "schemas"

    def __init__(self, registry: GeneratorRegistry) -> None:
        self.registry = registry
        self.schemas = self.load_schemas()
        self.validate_coverage()

    def load_schemas(self) -> dict:
        schemas = {}
        for path in sorted(self.SCHEMA_DIR.glob("*.yaml")):
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
            if not isinstance(data, dict):
                raise RuntimeError(f"schema YAML must be a mapping: {path}")
            generator_name = data.get("type")
            if not generator_name:
                raise RuntimeError(f"schema YAML missing type: {path}")
            if generator_name in schemas:
                raise RuntimeError(f"duplicate schema generator: {generator_name}")
            schemas[generator_name] = data
        return schemas

    def validate_coverage(self) -> None:
        registered = set(self.registry.MODULES)
        indexed = set(self.schemas)
        missing = sorted(registered - indexed)
        extra = sorted(indexed - registered)
        if missing or extra:
            raise RuntimeError(
                f"schema coverage mismatch missing={missing} extra={extra}"
            )

    def get_schema(self, generator_name: str) -> ApiEnvelope:
        if not self.registry.contains(generator_name):
            raise AppError(f"unknown generator: {generator_name}", status_code=404)
        if generator_name not in self.schemas:
            raise AppError(
                f"schema missing for generator: {generator_name}",
                status_code=500,
            )
        return ApiEnvelope.ok(data=self.schemas[generator_name])
