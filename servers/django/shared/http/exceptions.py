from rest_framework import status
from rest_framework.exceptions import APIException
from rest_framework.views import exception_handler

from shared.http.response import api_error


class AppError(APIException):
    status_code = status.HTTP_400_BAD_REQUEST
    default_detail = "Bad request"
    default_code = "error"

    def __init__(self, message, status_code=None):
        self.status_code = status_code or self.status_code
        self.detail = message
        super().__init__(detail=message)


def api_exception_handler(exc, context):
    response = exception_handler(exc, context)
    if response is None:
        return response
    return api_error(
        message=format_error_message(response.data),
        status=response.status_code,
    )


def format_error_message(detail):
    parts = []
    if isinstance(detail, dict):
        for key, value in detail.items():
            if key == "detail":
                parts.append(str(value))
                continue
            if isinstance(value, (list, tuple)):
                parts.extend(f"{key}: {item}" for item in value)
            else:
                parts.append(f"{key}: {value}")
    elif isinstance(detail, list):
        parts = [str(item) for item in detail]
    elif detail is not None:
        parts = [str(detail)]
    return "; ".join(parts) or "Bad request"
