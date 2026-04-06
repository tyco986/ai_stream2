import time
import uuid

import structlog
from structlog.contextvars import clear_contextvars

logger = structlog.get_logger("access")


class RequestIDMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        request_id = request.headers.get("X-Request-ID", str(uuid.uuid4()))
        request.request_id = request_id
        structlog.contextvars.bind_contextvars(request_id=request_id)
        try:
            response = self.get_response(request)
            response["X-Request-ID"] = request_id
            return response
        finally:
            clear_contextvars()


class AccessLogMiddleware:
    """Log every HTTP request with method, path, status, and duration."""

    SKIP_PATHS = ("/api/v1/health/live/", "/api/v1/health/ready/")

    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        if request.path in self.SKIP_PATHS:
            return self.get_response(request)

        start = time.monotonic()
        response = self.get_response(request)
        duration_ms = (time.monotonic() - start) * 1000

        logger.info(
            "http_request",
            method=request.method,
            path=request.path,
            status=response.status_code,
            duration_ms=round(duration_ms, 1),
            user=str(getattr(request, "user", None)),
        )
        return response
