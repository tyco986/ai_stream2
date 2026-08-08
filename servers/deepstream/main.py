import argparse
import logging
import time
from contextlib import asynccontextmanager
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
    DEFAULT_PORT,
    LOG_FORMAT,
    LOG_ROOT,
    LOGGER_NAME,
)
from utils.api.routes import router
from utils.api.schemas import ApiEnvelope
from utils.api.services import AppError, PipelineStartService, SchemaService


def configure_logging(log_root: Path = LOG_ROOT) -> None:
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


@asynccontextmanager
async def lifespan(app: FastAPI):
    pipeline_service = PipelineStartService()
    app.state.pipeline_service = pipeline_service
    app.state.schema_service = SchemaService(PipelineStartService.PIPELINE_MODULES)
    logger = logging.getLogger(LOGGER_NAME)
    types = ", ".join(sorted(PipelineStartService.PIPELINE_MODULES))
    logger.info("DeepStream API started log_root=%s types=%s", LOG_ROOT, types)
    yield
    logger.info("DeepStream API stopped")


async def handle_app_error(request: Request, exc: AppError) -> JSONResponse:
    return JSONResponse(
        status_code=exc.status_code,
        content=ApiEnvelope(success=False, message=exc.message).model_dump(),
    )


async def handle_http_exception(request: Request, exc: HTTPException) -> JSONResponse:
    detail = exc.detail if isinstance(exc.detail, str) else str(exc.detail)
    return JSONResponse(
        status_code=exc.status_code,
        content=ApiEnvelope(success=False, message=detail).model_dump(),
    )


async def handle_validation_error(
    request: Request, exc: RequestValidationError
) -> JSONResponse:
    return JSONResponse(
        status_code=422,
        content=ApiEnvelope(success=False, message=str(exc.errors())).model_dump(),
    )


async def handle_unhandled(request: Request, exc: Exception) -> JSONResponse:
    logging.getLogger(LOGGER_NAME).exception("unhandled error path=%s", request.url.path)
    return JSONResponse(
        status_code=500,
        content=ApiEnvelope(success=False, message="internal error").model_dump(),
    )


def create_app() -> FastAPI:
    configure_logging()
    app = FastAPI(title="DeepStream API", version="1.0.0", lifespan=lifespan)
    app.add_middleware(RequestLogMiddleware)
    app.add_exception_handler(AppError, handle_app_error)
    app.add_exception_handler(HTTPException, handle_http_exception)
    app.add_exception_handler(RequestValidationError, handle_validation_error)
    app.add_exception_handler(Exception, handle_unhandled)
    app.include_router(router)
    return app


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="DeepStream API service")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    uvicorn.run(create_app(), host=args.host, port=args.port)


if __name__ == "__main__":
    main()
