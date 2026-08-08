from fastapi import APIRouter, Depends, Form
from starlette.requests import Request

from utils.api.constants import DEFAULT_PRECISION, PROJECT_NAME
from utils.api.schemas import ApiEnvelope
from utils.api.services import ExportEngineService

router = APIRouter(prefix=f"/{PROJECT_NAME}/export_trt", tags=["export_trt"])


def get_export_engine(request: Request) -> ExportEngineService:
    return request.app.state.export_engine


@router.get("/health", response_model=ApiEnvelope, summary="Health check")
def health() -> ApiEnvelope:
    return ApiEnvelope.ok()


@router.post(
    "/export_engine",
    response_model=ApiEnvelope,
    summary="Export TensorRT engine from ONNX folder",
)
def route_export_engine(
    input: str = Form(...),
    batch_size: int | None = Form(None),
    gpu_id: int = Form(0),
    precision: str = Form(DEFAULT_PRECISION),
    opt_level: int | None = Form(None),
    svc: ExportEngineService = Depends(get_export_engine),
) -> ApiEnvelope:
    return svc.export_engine(
        input, batch_size, gpu_id, precision, opt_level
    )
