import argparse
import logging
import os
import threading
import time
import traceback
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
from fastapi import FastAPI
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

from utils.pipeline_runner import PipelineRunner
from utils.yolo_pipeline.yolo_pipeline import YoloPipeline

PROJECT_NAME = "ai_stream2"
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
DEFAULT_LOG_ROOT = Path(os.environ.get("LOG_ROOT", "/root/logs/deepstream"))
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8092"))


class ApiResponse(BaseModel):
    success: bool
    message: str = ""


class BuildPipelineRequest(BaseModel):
    input: str = Field(..., description="Generator config directory path")
    name: str = Field(..., description="Pipeline instance name")


class PipelineNameRequest(BaseModel):
    name: str = Field(..., description="Pipeline instance name")


class RequestLogMiddleware(BaseHTTPMiddleware):
    def __init__(self, app, logger: logging.Logger) -> None:
        super().__init__(app)
        self.log = logger

    async def dispatch(self, request: Request, call_next) -> Response:
        start = time.perf_counter()
        method, path = request.method, request.url.path
        self.log.info("request start %s %s", method, path)
        response = await call_next(request)
        self.log.info(
            "request %s %s -> %s (%.1f ms)",
            method,
            path,
            response.status_code,
            (time.perf_counter() - start) * 1000,
        )
        return response


def configure_logger(log_root: Path, name: str = "deepstream_api") -> logging.Logger:
    log_root.mkdir(parents=True, exist_ok=True)
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    logger.handlers.clear()
    logger.propagate = False
    handler = RotatingFileHandler(
        log_root / "app.log",
        maxBytes=1024 * 1024,
        backupCount=10,
        encoding="utf-8",
    )
    handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(handler)
    return logger


def ok(message: str = "") -> JSONResponse:
    return JSONResponse(ApiResponse(success=True, message=message).model_dump())


def fail(message: str, status_code: int = 400) -> JSONResponse:
    return JSONResponse(
        status_code=status_code,
        content=ApiResponse(success=False, message=message).model_dump(),
    )


class DeepstreamServer:
    def __init__(self, log_root: Path) -> None:
        self.logger = configure_logger(log_root)
        self.lock = threading.Lock()
        self.pipelines: dict[str, dict] = {}

        self.app = FastAPI(
            title="DeepStream API",
            description="Build and run static YOLO detection pipelines. See servers/deepstream/README.md",
            version="1.0.0",
        )
        self.app.add_middleware(RequestLogMiddleware, logger=self.logger)
        self.app.add_exception_handler(
            RequestValidationError,
            lambda _, exc: fail(str(exc.errors()), status_code=422),
        )
        prefix = f"/{PROJECT_NAME}/deepstream"
        self.app.add_api_route(
            f"{prefix}/build_pipeline",
            self.build_pipeline,
            methods=["POST"],
            summary="Load pipeline YAML and attach drawer probe",
            response_model=ApiResponse,
        )
        self.app.add_api_route(
            f"{prefix}/start_pipeline",
            self.start_pipeline,
            methods=["POST"],
            summary="Start a built pipeline in background",
            response_model=ApiResponse,
        )
        self.app.add_api_route(
            f"{prefix}/stop_pipeline",
            self.stop_pipeline,
            methods=["POST"],
            summary="Stop a running pipeline",
            response_model=ApiResponse,
        )
        self.logger.info("DeepStream API initialized log_root=%s", log_root)

    def is_running(self, name: str) -> bool:
        entry = self.pipelines.get(name)
        thread = entry and entry.get("thread")
        return thread is not None and thread.is_alive()

    def build_pipeline(self, body: BuildPipelineRequest) -> JSONResponse:
        config_dir = Path(body.input).expanduser().resolve()
        if not config_dir.is_dir():
            return fail(f"config dir not found: {config_dir}")
        pipeline_yml = config_dir / "pipeline.yml"
        if not pipeline_yml.is_file():
            return fail(f"pipeline.yml not found in {config_dir}")

        with self.lock:
            if self.is_running(body.name):
                return fail(f"pipeline already running: {body.name}")
            try:
                yolo_pipeline = YoloPipeline(str(config_dir), body.name)
                pipeline = yolo_pipeline.build(logger=self.logger)
                self.pipelines[body.name] = {
                    "config_dir": str(config_dir),
                    "yolo_task": yolo_pipeline.yolo_task,
                    "pipeline": pipeline,
                    "runner": PipelineRunner(pipeline),
                    "thread": None,
                }
                self.logger.info(
                    "built pipeline name=%s config_dir=%s task=%s",
                    body.name,
                    config_dir,
                    yolo_pipeline.yolo_task,
                )
                return ok()
            except Exception:
                message = traceback.format_exc()
                self.logger.error("build_pipeline failed name=%s\n%s", body.name, message)
                return fail(message)

    def start_pipeline(self, body: PipelineNameRequest) -> JSONResponse:
        with self.lock:
            entry = self.pipelines.get(body.name)
            if entry is None:
                return fail(f"pipeline not built: {body.name}")
            if self.is_running(body.name):
                return fail(f"pipeline already running: {body.name}")

            def run_pipeline() -> None:
                self.logger.info("pipeline start name=%s", body.name)
                try:
                    entry["runner"].start()
                except Exception:
                    self.logger.error(
                        "pipeline error name=%s\n%s",
                        body.name,
                        traceback.format_exc(),
                    )
                finally:
                    self.logger.info("pipeline exit name=%s", body.name)

            thread = threading.Thread(target=run_pipeline, name=f"ds-{body.name}", daemon=True)
            entry["thread"] = thread
            thread.start()
            return ok("started")

    def stop_pipeline(self, body: PipelineNameRequest) -> JSONResponse:
        with self.lock:
            entry = self.pipelines.get(body.name)
            if entry is None:
                return fail(f"pipeline not built: {body.name}")
            if not self.is_running(body.name):
                return fail(f"pipeline not running: {body.name}")

            try:
                entry["runner"].stop()
                entry["thread"].join(timeout=30)
                entry["thread"] = None
                self.logger.info("pipeline stopped name=%s", body.name)
                return ok("stopped")
            except Exception:
                message = traceback.format_exc()
                self.logger.error("stop_pipeline failed name=%s\n%s", body.name, message)
                return fail(message)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="DeepStream API service")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--log-root", default=str(DEFAULT_LOG_ROOT))
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = DeepstreamServer(log_root=Path(args.log_root))
    uvicorn.run(server.app, host=args.host, port=args.port)
