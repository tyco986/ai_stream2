import argparse
import logging
import os
import time
import traceback
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
import yaml
from fastapi import FastAPI, File, UploadFile
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

from utils.manager import GeneratorManager

PROJECT_NAME = os.environ.get("PROJECT_NAME", "ai_stream2")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
DEFAULT_LOG_ROOT = Path(os.environ.get("LOG_ROOT", "/root/logs/generator"))
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8091"))


class ApiErrorResponse(BaseModel):
    success: bool = False
    message: str


class ApiJsonResponse(BaseModel):
    success: bool = True
    message: str = ""


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


def configure_file_logger(log_root: Path, name: str = "generator_api") -> logging.Logger:
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


async def handle_validation_error(
    _: Request, exc: RequestValidationError
) -> JSONResponse:
    return error_response(str(exc.errors()), status_code=422)


class GeneratorServer:
    def __init__(self, log_root: Path) -> None:
        self.logger = configure_file_logger(log_root)
        generators = ", ".join(sorted(GeneratorManager.GENERATORS))

        self.app = FastAPI(
            title="Generator API",
            description=(
                f"Upload generator YAML as multipart `input`. "
                f"Generators: {generators}. Details: servers/generator/README.md"
            ),
            version="1.0.0",
        )
        self.app.add_middleware(RequestLogMiddleware, logger=self.logger)
        self.app.add_exception_handler(
            RequestValidationError, handle_validation_error
        )
        self.app.add_api_route(
            f"/{PROJECT_NAME}/generator/generate",
            self.generate,
            methods=["POST"],
            summary="Generate DeepStream pipeline configs",
            response_model=ApiJsonResponse,
            responses={400: {"model": ApiErrorResponse}, 422: {"model": ApiErrorResponse}},
        )
        self.logger.info("Generator API initialized log_root=%s", log_root)

    async def generate(self, input: UploadFile = File(...)) -> Response:
        try:
            GeneratorManager(yaml.safe_load(await input.read())).write()
            return JSONResponse(ApiJsonResponse().model_dump())
        except Exception:
            message = traceback.format_exc()
            self.logger.error("generate failed\n%s", message)
            return error_response(message)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generator API service")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--log-root", default=str(DEFAULT_LOG_ROOT))
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = GeneratorServer(log_root=Path(args.log_root))
    uvicorn.run(server.app, host=args.host, port=args.port)
