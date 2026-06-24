import argparse
import logging
import os
import time
import traceback
from collections.abc import Callable
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
from fastapi import FastAPI, Form
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

from utils.export_engine import DEFAULT_PRECISION, ExportEngineRunner

PROJECT_NAME = "ai_stream2"
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
DEFAULT_MODEL_ROOT = Path("/root/models")
DEFAULT_LOG_ROOT = Path(os.environ.get("LOG_ROOT", "/root/logs/exporttrt"))
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "9000"))


class ApiErrorResponse(BaseModel):
    success: bool = False
    message: str
    output: None = None


class ApiJsonResponse(BaseModel):
    success: bool = True
    message: str = ""
    output: None = None


class RequestLogMiddleware(BaseHTTPMiddleware):
    def __init__(self, app, logger: logging.Logger) -> None:
        super().__init__(app)
        self.log = logger

    async def dispatch(self, request: Request, call_next) -> Response:
        start = time.perf_counter()
        method, path = request.method, request.url.path
        self.log.info("request start %s %s", method, path)
        response = await call_next(request)
        elapsed_ms = (time.perf_counter() - start) * 1000
        self.log.info(
            "request %s %s -> %s (%.1f ms)",
            method,
            path,
            response.status_code,
            elapsed_ms,
        )
        return response


def configure_file_logger(log_root: Path, name: str = "exporttrt_api") -> logging.Logger:
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


def error_response(message: str, status_code: int = 400) -> JSONResponse:
    return JSONResponse(
        status_code=status_code,
        content=ApiErrorResponse(message=message).model_dump(),
    )


def json_ok(message: str = "") -> dict:
    return ApiJsonResponse(message=message).model_dump()


async def handle_validation_error(
    request: Request, exc: RequestValidationError
) -> JSONResponse:
    return error_response(str(exc.errors()), status_code=422)


class ExportTrtServer:
    def __init__(
        self,
        log_root: Path,
        model_root: Path = DEFAULT_MODEL_ROOT,
    ) -> None:
        self.logger = configure_file_logger(log_root)
        self.model_root = model_root
        self.input_root = model_root / "onnx"
        self.output_root = model_root / "trt"
        self.runner = ExportEngineRunner(self.logger, model_root)
        self.input_root.mkdir(parents=True, exist_ok=True)
        self.output_root.mkdir(parents=True, exist_ok=True)

        self.app = FastAPI(
            title="Export TRT API",
            description=(
                f"Read ONNX folder from input path, export TensorRT engine to {self.output_root}/{{name}}/. "
                "Details: servers/exporttrt/README.md"
            ),
            version="1.0.0",
        )
        self.app.add_middleware(RequestLogMiddleware, logger=self.logger)
        self.app.add_exception_handler(
            RequestValidationError, handle_validation_error
        )
        self.register_routes()
        self.logger.info(
            "Export TRT API initialized log_root=%s model_root=%s",
            log_root,
            model_root,
        )

    def register_routes(self) -> None:
        prefix = f"/{PROJECT_NAME}/exporttrt"
        self.app.add_api_route(
            f"{prefix}/export_engine",
            self.export_engine,
            methods=["POST"],
            summary="Export TensorRT engine from ONNX folder",
            response_model=ApiJsonResponse,
            responses={400: {"model": ApiErrorResponse}, 422: {"model": ApiErrorResponse}},
        )

    def handle(self, route: str, action: Callable[[], dict]) -> Response:
        try:
            return JSONResponse(action())
        except Exception:
            message = traceback.format_exc()
            self.logger.error("%s failed\n%s", route, message)
            return error_response(message)

    def export_engine(
        self,
        input: str = Form(...),
        batch_size: int | None = Form(None),
        gpu_id: int = Form(0),
        precision: str = Form(DEFAULT_PRECISION),
    ) -> Response:
        return self.handle(
            "export_engine",
            lambda: self.run_export_engine(input, batch_size, gpu_id, precision),
        )

    def run_export_engine(
        self,
        input_path: str,
        batch_size: int | None,
        gpu_id: int,
        precision: str,
    ) -> dict:
        bundle_dir = self.runner.run(input_path, batch_size, gpu_id, precision)
        return json_ok(message=str(bundle_dir))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export TRT API service")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--log-root", default=str(DEFAULT_LOG_ROOT))
    parser.add_argument("--model-root", default=str(DEFAULT_MODEL_ROOT))
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = ExportTrtServer(
        log_root=Path(args.log_root),
        model_root=Path(args.model_root),
    )
    uvicorn.run(server.app, host=args.host, port=args.port)
