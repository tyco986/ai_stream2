from rest_framework.response import Response
from rest_framework.views import exception_handler


class ServiceError(Exception):
    """业务异常基类。

    code 是业务错误码（字符串），前端用于精确匹配错误类型。
    http_status 是 HTTP 状态码。二者独立。
    """

    def __init__(self, message, code="UNKNOWN_ERROR", http_status=400):
        self.message = message
        self.code = code
        self.http_status = http_status


class DeepStreamUnavailableError(ServiceError):
    def __init__(self):
        super().__init__(
            "DeepStream 服务不可用",
            code="DEEPSTREAM_UNAVAILABLE",
            http_status=503,
        )


class CameraNotFoundError(ServiceError):
    def __init__(self, camera_id):
        super().__init__(
            f"摄像头 {camera_id} 不存在",
            code="CAMERA_NOT_FOUND",
            http_status=404,
        )


class CameraAlreadyDeletedError(ServiceError):
    def __init__(self, camera_id):
        super().__init__(
            f"摄像头 {camera_id} 已被删除",
            code="CAMERA_ALREADY_DELETED",
            http_status=400,
        )


class InvalidStateTransitionError(ServiceError):
    def __init__(self, current_status, action):
        super().__init__(
            f"当前状态 {current_status} 不允许执行 {action}",
            code="INVALID_STATE_TRANSITION",
            http_status=400,
        )


class DeploymentError(ServiceError):
    def __init__(self, message):
        super().__init__(
            message,
            code="DEPLOYMENT_ERROR",
            http_status=500,
        )


# ---------------------------------------------------------------------------
# DRF global exception handler
# ---------------------------------------------------------------------------

_STATUS_CODE_TO_CODE = {
    400: "VALIDATION_ERROR",
    401: "UNAUTHORIZED",
    403: "FORBIDDEN",
    404: "NOT_FOUND",
    405: "METHOD_NOT_ALLOWED",
    429: "THROTTLED",
    500: "SERVER_ERROR",
    503: "SERVICE_UNAVAILABLE",
}


def custom_exception_handler(exc, context):
    if isinstance(exc, ServiceError):
        return Response(
            {"code": exc.code, "message": exc.message, "data": None},
            status=exc.http_status,
        )

    response = exception_handler(exc, context)
    if response is not None:
        code = _STATUS_CODE_TO_CODE.get(response.status_code, "SERVER_ERROR")
        data = response.data if response.status_code in (400, 401, 403, 404, 429) else None
        return Response(
            {"code": code, "message": str(exc), "data": data},
            status=response.status_code,
        )

    return None
