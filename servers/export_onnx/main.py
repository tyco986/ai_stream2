import time

import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import ValidationError
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

from utils.api.constants import HOST, PORT
from utils.api.logger import RequestLogger
from utils.api.routes import router
from utils.api.schemas import ApiEnvelope, AppError


class RequestLogMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next) -> Response:
        started = time.perf_counter()
        response = await call_next(request)
        chunks = [chunk async for chunk in response.body_iterator]
        body = b"".join(chunks)
        latency_ms = (time.perf_counter() - started) * 1000
        content_length = request.headers.get("content-length")
        in_bytes = int(content_length) if content_length and content_length.isdigit() else 0
        RequestLogger().write(request, response.status_code, body, latency_ms, "", in_bytes)
        return Response(
            content=body,
            status_code=response.status_code,
            headers=dict(response.headers),
            media_type=response.media_type,
        )


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
    return fail_json(500, "internal error")


def create_app() -> FastAPI:
    app = FastAPI(title="Export ONNX API", version="1.0.0")
    app.add_middleware(RequestLogMiddleware)
    app.add_exception_handler(AppError, handle_app_error)
    app.add_exception_handler(ValueError, handle_value_error)
    app.add_exception_handler(HTTPException, handle_http_exception)
    app.add_exception_handler(RequestValidationError, handle_validation_error)
    app.add_exception_handler(ValidationError, handle_pydantic_error)
    app.add_exception_handler(Exception, handle_unhandled)
    app.include_router(router)
    return app


def main() -> None:
    uvicorn.run(create_app(), host=HOST, port=PORT)


if __name__ == "__main__":
    main()
