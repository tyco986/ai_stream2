import argparse
import logging
import time
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

from utils.api.constants import (
    DEFAULT_HOST,
    DEFAULT_LOG_ROOT,
    DEFAULT_MODEL_ROOT,
    DEFAULT_PORT,
    LOG_FORMAT,
    LOGGER_NAME,
)
from utils.api.routes import router
from utils.api.schemas import ApiErrorResponse
from utils.api.services import AppError, ExportRunner, ExportService


def configure_logging(log_root: Path = DEFAULT_LOG_ROOT) -> None:
    log_root.mkdir(parents=True, exist_ok=True)
    logger = logging.getLogger(LOGGER_NAME)
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


class RequestLogMiddleware(BaseHTTPMiddleware):
    def __init__(self, app) -> None:
        super().__init__(app)
        self.log = logging.getLogger(LOGGER_NAME)

    async def dispatch(self, request: Request, call_next) -> Response:
        start = time.perf_counter()
        method = request.method
        path = request.url.path
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


async def handle_app_error(request: Request, exc: AppError) -> JSONResponse:
    return JSONResponse(
        status_code=exc.status_code,
        content=ApiErrorResponse(message=exc.message).model_dump(),
    )


async def handle_http_exception(request: Request, exc: HTTPException) -> JSONResponse:
    detail = exc.detail if isinstance(exc.detail, str) else str(exc.detail)
    return JSONResponse(
        status_code=exc.status_code,
        content=ApiErrorResponse(message=detail).model_dump(),
    )


async def handle_validation_error(
    request: Request, exc: RequestValidationError
) -> JSONResponse:
    return JSONResponse(
        status_code=422,
        content=ApiErrorResponse(message=str(exc.errors())).model_dump(),
    )


async def handle_unhandled(request: Request, exc: Exception) -> JSONResponse:
    logging.getLogger(LOGGER_NAME).exception("unhandled error path=%s", request.url.path)
    return JSONResponse(
        status_code=500,
        content=ApiErrorResponse(message="internal error").model_dump(),
    )


def create_app(
    log_root: Path = DEFAULT_LOG_ROOT,
    model_root: Path = DEFAULT_MODEL_ROOT,
) -> FastAPI:
    configure_logging(log_root)
    app = FastAPI(
        title="Export ONNX API",
        description=(
            f"Upload .pt to {model_root}/pt/, export ONNX to {model_root}/onnx/{{name}}/. "
            "Details: servers/export_onnx/README.md"
        ),
        version="1.0.0",
    )
    app.state.export_service = ExportService(ExportRunner(model_root))
    app.add_middleware(RequestLogMiddleware)
    app.add_exception_handler(AppError, handle_app_error)
    app.add_exception_handler(HTTPException, handle_http_exception)
    app.add_exception_handler(RequestValidationError, handle_validation_error)
    app.add_exception_handler(Exception, handle_unhandled)
    app.include_router(router)
    logging.getLogger(LOGGER_NAME).info(
        "ExportOnnx API initialized log_root=%s model_root=%s",
        log_root,
        model_root,
    )
    return app


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export ONNX API")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    app = create_app()
    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
