import argparse
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import ValidationError
from starlette.requests import Request

from utils.api.constants import (
    DEFAULT_HOST,
    DEFAULT_LOG_ROOT,
    DEFAULT_PORT,
    LOG_FORMAT,
    LOGGER_NAME,
)
from utils.api.routes import router
from utils.api.schemas import ApiEnvelope, AppError


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


def fail_json(status_code: int, message: str) -> JSONResponse:
    return JSONResponse(
        status_code=status_code,
        content=ApiEnvelope.fail(message).model_dump(),
    )


async def handle_app_error(request: Request, exc: AppError) -> JSONResponse:
    return fail_json(exc.status_code, exc.message)


async def handle_value_error(request: Request, exc: ValueError) -> JSONResponse:
    return fail_json(400, str(exc))


async def handle_http_exception(request: Request, exc: HTTPException) -> JSONResponse:
    detail = exc.detail if isinstance(exc.detail, str) else str(exc.detail)
    return fail_json(exc.status_code, detail)


async def handle_validation_error(
    request: Request, exc: RequestValidationError
) -> JSONResponse:
    return fail_json(422, str(exc.errors()))


async def handle_pydantic_error(request: Request, exc: ValidationError) -> JSONResponse:
    return fail_json(400, str(exc.errors()))


async def handle_unhandled(request: Request, exc: Exception) -> JSONResponse:
    logging.getLogger(LOGGER_NAME).exception("unhandled error path=%s", request.url.path)
    return fail_json(500, "internal error")


def create_app() -> FastAPI:
    configure_logging()
    app = FastAPI(title="Export TRT API", version="1.0.0")
    app.add_exception_handler(AppError, handle_app_error)
    app.add_exception_handler(ValueError, handle_value_error)
    app.add_exception_handler(HTTPException, handle_http_exception)
    app.add_exception_handler(RequestValidationError, handle_validation_error)
    app.add_exception_handler(ValidationError, handle_pydantic_error)
    app.add_exception_handler(Exception, handle_unhandled)
    app.include_router(router)
    return app


def main() -> None:
    parser = argparse.ArgumentParser(description="Export TRT API")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()
    uvicorn.run(create_app(), host=args.host, port=args.port)


if __name__ == "__main__":
    main()
