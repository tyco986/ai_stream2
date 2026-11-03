import time
import traceback

import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import ValidationError
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

from utils.api.constants import HOST, LOG_HEADERS, PORT, STATE_INPUT
from utils.api.logger import RequestLogger
from utils.api.routes import router
from utils.api.schemas import ApiEnvelope, AppError


def fail_json(status_code: int, message: str) -> JSONResponse:
    payload = JSONResponse(
        status_code=status_code,
        content=ApiEnvelope.fail(message).model_dump(),
    )
    return payload


def app_error_response(exc: AppError) -> JSONResponse:
    payload = fail_json(exc.status_code, exc.message)
    return payload


def value_error_response(exc: ValueError) -> JSONResponse:
    payload = fail_json(400, str(exc))
    return payload


def http_exception_response(exc: HTTPException) -> JSONResponse:
    detail = exc.detail if isinstance(exc.detail, str) else str(exc.detail)
    payload = fail_json(exc.status_code, detail)
    return payload


def validation_error_response(exc: RequestValidationError) -> JSONResponse:
    payload = fail_json(422, str(exc.errors()))
    return payload


def pydantic_error_response(exc: ValidationError) -> JSONResponse:
    payload = fail_json(400, str(exc.errors()))
    return payload


ERROR_RESPONSES = {
    AppError: app_error_response,
    RequestValidationError: validation_error_response,
    ValidationError: pydantic_error_response,
    ValueError: value_error_response,
    HTTPException: http_exception_response,
}


def error_response(exc: Exception) -> JSONResponse:
    payload = fail_json(500, "internal error")
    for cls in type(exc).__mro__:
        handler = ERROR_RESPONSES.get(cls)
        if handler is not None:
            payload = handler(exc)
            break
    return payload


class RequestLogMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next) -> Response:
        started = time.perf_counter()
        error = ""
        try:
            response = await call_next(request)
        except Exception as exc:
            error = traceback.format_exc()
            response = error_response(exc)
        body = await self.response_body(response)
        latency_ms = round((time.perf_counter() - started) * 1000, 2)
        content_length = request.headers.get("content-length")
        in_bytes = int(content_length) if content_length and content_length.isdigit() else 0
        logger = RequestLogger()
        header = {name: request.headers[name] for name in LOG_HEADERS if name in request.headers}
        client = request.client.host if request.client is not None else ""
        logger.write(
            request.method,
            str(request.url),
            response.status_code,
            getattr(request.state, STATE_INPUT, ""),
            logger.output_value(body),
            latency_ms,
            client,
            in_bytes,
            len(body),
            error,
            header,
        )
        payload = Response(
            content=body,
            status_code=response.status_code,
            headers=dict(response.headers),
            media_type=response.media_type,
        )
        return payload

    async def response_body(self, response: Response) -> bytes:
        iterator = getattr(response, "body_iterator", None)
        chunks = [chunk async for chunk in iterator] if iterator is not None else [response.body]
        body = b"".join(chunks)
        return body


def create_app() -> FastAPI:
    app = FastAPI(title="Export ONNX API", version="1.0.0")
    app.include_router(router)
    app.router.middleware_stack = RequestLogMiddleware(app.router.middleware_stack)
    return app


def main() -> None:
    uvicorn.run(create_app(), host=HOST, port=PORT)


if __name__ == "__main__":
    main()
