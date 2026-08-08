from fastapi import APIRouter, Depends, File, Form, UploadFile
from starlette.requests import Request

from utils.api.constants import DEFAULT_CONF, DEFAULT_IOU, EXPORT_SPECS, PROJECT_NAME
from utils.api.schemas import ApiErrorResponse, ApiJsonResponse
from utils.api.services import ExportService

router = APIRouter(prefix=f"/{PROJECT_NAME}/export_onnx", tags=["export_onnx"])


def get_export(request: Request) -> ExportService:
    return request.app.state.export_service


@router.get(
    "/health",
    response_model=ApiJsonResponse,
    summary="Health check",
)
def health() -> ApiJsonResponse:
    return ApiJsonResponse.ok()


@router.get(
    "/types",
    response_model=ApiJsonResponse,
    summary="List available export routes (label, route, family, task)",
)
def route_types(svc: ExportService = Depends(get_export)) -> ApiJsonResponse:
    return svc.list_types()


class ExportEndpoint:
    def __init__(self, route_name: str) -> None:
        self.route_name = route_name

    def __call__(
        self,
        input: UploadFile = File(...),
        size: int = Form(640),
        dynamic: bool = Form(False),
        simplify: bool = Form(False),
        batch: int = Form(1),
        opset: int = Form(18),
        max_det: int | None = Form(None),
        conf: float = Form(DEFAULT_CONF),
        iou: float = Form(DEFAULT_IOU),
        svc: ExportService = Depends(get_export),
    ) -> ApiJsonResponse:
        return svc.export(
            self.route_name,
            input,
            size,
            dynamic,
            simplify,
            batch,
            opset,
            max_det,
            conf,
            iou,
        )


for route_name in EXPORT_SPECS:
    endpoint = ExportEndpoint(route_name)
    router.add_api_route(
        f"/{route_name}",
        endpoint,
        methods=["POST"],
        summary=f"Export {route_name}",
        response_model=ApiJsonResponse,
        responses={
            400: {"model": ApiErrorResponse},
            422: {"model": ApiErrorResponse},
        },
        name=route_name,
    )
