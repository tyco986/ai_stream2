import importlib
import logging
import threading
from pathlib import Path

import yaml
from fastapi import UploadFile
from pydantic import ValidationError

from utils.api.constants import (
    BASE_PIPELINE_TYPES,
    CONFIG_SAVE_DIR,
    LOG_ROOT,
    LOGGER_NAME,
    PRESENCE_PIPELINE_TYPES,
)
from utils.api.schemas import ApiEnvelope, StartPipelineRequest


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class PipelineStartService:
    # name -> importable module path (lazy-loaded on start)
    PIPELINE_MODULES = {
        "BaseImagePipeline": "utils.base_pipeline.base_image",
        "BaseVideoPipeline": "utils.base_pipeline.base_video",
        "BaseRTSPPipeline": "utils.base_pipeline.base_rtsp",
        "DetVisRTSPPipeline": "utils.yolo_pipeline.det_vis_rtsp",
        "SegVisRTSPPipeline": "utils.yolo_pipeline.seg_vis_rtsp",
        "DetImagePipeline": "utils.yolo_pipeline.det_image",
        "Pose2DImagePipeline": "utils.pose_pipeline.pose2d_image",
        "Pose2DVideoPipeline": "utils.pose_pipeline.pose2d_video",
        "Pose2DRTSPPipeline": "utils.pose_pipeline.pose2d_rtsp",
        "Pose2DVisRTSPPipeline": "utils.pose_pipeline.pose2d_vis_rtsp",
        "SegImagePipeline": "utils.yolo_pipeline.seg_image",
        "SegSahiImagePipeline": "utils.yolo_pipeline.seg_sahi_image",
        "SegSahiVideoPipeline": "utils.yolo_pipeline.seg_sahi_video",
        "SegSahiVisRTSPPipeline": "utils.yolo_pipeline.seg_sahi_vis_rtsp",
        "DetVideoPipeline": "utils.yolo_pipeline.det_video",
        "SegVideoPipeline": "utils.yolo_pipeline.seg_video",
        "DetSahiVisRTSPPipeline": "utils.yolo_pipeline.det_sahi_vis_rtsp",
        "DetSahiImagePipeline": "utils.yolo_pipeline.det_sahi_image",
        "DetSahiVideoPipeline": "utils.yolo_pipeline.det_sahi_video",
        "PresenceRTSPPipeline": "utils.event_pipeline.presence_rtsp",
        "PresenceVideoPipeline": "utils.event_pipeline.presence_video",
    }

    def __init__(self) -> None:
        self.pipeline = None
        self.runner = None
        self.runner_thread = None
        self.pipeline_name = None
        self.pipeline_type = None
        self.logger = logging.getLogger(LOGGER_NAME)
        self.loaded = {}

    def is_running(self) -> bool:
        return self.runner_thread is not None and self.runner_thread.is_alive()

    def get_status(self) -> ApiEnvelope:
        data = {
            "pipeline_running": self.is_running(),
            "name": self.pipeline_name,
            "type": self.pipeline_type,
        }
        return ApiEnvelope.ok(data=data)

    def list_types(self) -> ApiEnvelope:
        items = [{"pipeline": name} for name in sorted(self.PIPELINE_MODULES)]
        return ApiEnvelope.ok(data={"items": items})

    def resolve_pipeline(self, pipeline_type: str):
        cls = self.loaded.get(pipeline_type)
        if cls is None:
            module = importlib.import_module(self.PIPELINE_MODULES[pipeline_type])
            cls = getattr(module, pipeline_type)
            self.loaded[pipeline_type] = cls
        return cls

    def build_pipeline_kwargs(self, body: StartPipelineRequest, logger: dict) -> dict:
        kwargs = {
            "logger": logger,
            "messager": body.messager,
        }
        if body.type in PRESENCE_PIPELINE_TYPES:
            if body.debouncer is not None:
                kwargs["debouncer"] = body.debouncer
            if body.drawer is not None:
                kwargs["drawer"] = body.drawer
            if body.capturer is not None:
                kwargs["capturer"] = body.capturer
        elif body.drawer is not None:
            kwargs["drawer"] = body.drawer
        return kwargs

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

        try:
            body = StartPipelineRequest.model_validate(config)
        except ValidationError as exc:
            raise AppError(str(exc.errors())) from exc

        if self.is_running():
            raise AppError("pipeline is running")
        if body.type not in self.PIPELINE_MODULES:
            raise AppError(
                f"unknown pipeline type: {body.type!r} "
                f"(supported: {', '.join(sorted(self.PIPELINE_MODULES))})"
            )

        logger = dict(body.logger)
        logger["root"] = logger.get("root") or str(LOG_ROOT / body.name)
        builder_cls = self.resolve_pipeline(body.type)
        if body.type in BASE_PIPELINE_TYPES:
            builder = builder_cls(body.config_dir, body.name)
        else:
            builder = builder_cls(
                body.config_dir,
                body.name,
                **self.build_pipeline_kwargs(body, logger),
            )
        self.pipeline = builder.build()
        runner_cls = self.resolve_runner()
        self.runner = runner_cls(self.pipeline, logger=logger)
        self.pipeline_name = body.name
        self.pipeline_type = body.type
        self.runner_thread = threading.Thread(target=self.runner.start, daemon=True)
        self.runner_thread.start()

        self.logger.info("pipeline started name=%s type=%s", body.name, body.type)
        return ApiEnvelope.ok(data={"name": body.name, "type": body.type})

    def resolve_runner(self):
        cls = self.loaded.get("PipelineRunner")
        if cls is None:
            module = importlib.import_module("utils.base_pipeline.pipeline_runner")
            cls = getattr(module, "PipelineRunner")
            self.loaded["PipelineRunner"] = cls
        return cls


class SchemaService:
    SCHEMA_DIR = Path(__file__).resolve().parents[2] / "schemas"

    def __init__(self, pipelines: dict) -> None:
        self.pipelines = pipelines
        self.schemas = self.load_schemas()
        self.validate_coverage()

    def load_schemas(self) -> dict:
        schemas = {}
        for path in sorted(self.SCHEMA_DIR.glob("*.yaml")):
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
            if not isinstance(data, dict):
                raise RuntimeError(f"schema YAML must be a mapping: {path}")
            pipeline_type = data.get("type")
            if not pipeline_type:
                raise RuntimeError(f"schema YAML missing type: {path}")
            if pipeline_type in schemas:
                raise RuntimeError(f"duplicate schema type: {pipeline_type}")
            schemas[pipeline_type] = data
        return schemas

    def validate_coverage(self) -> None:
        registered = set(self.pipelines)
        indexed = set(self.schemas)
        missing = sorted(registered - indexed)
        extra = sorted(indexed - registered)
        if missing or extra:
            raise RuntimeError(
                f"schema coverage mismatch missing={missing} extra={extra}"
            )

    def get_schema(self, pipeline_type: str) -> ApiEnvelope:
        if pipeline_type not in self.pipelines:
            raise AppError(f"unknown pipeline type: {pipeline_type}", status_code=404)
        if pipeline_type not in self.schemas:
            raise AppError(
                f"schema missing for pipeline type: {pipeline_type}",
                status_code=500,
            )
        return ApiEnvelope.ok(data=self.schemas[pipeline_type])
