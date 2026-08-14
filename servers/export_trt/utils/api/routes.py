from fastapi import APIRouter, File, UploadFile

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope
from utils.api.services import ExportTrtService
from utils.manager.trt_exporter_manager import TrtExporterManager

router = APIRouter(prefix=f"/{PROJECT_NAME}/export_trt", tags=["export_trt"])

EXPORT_RESPONSES = {
    400: {"model": ApiEnvelope},
    422: {"model": ApiEnvelope},
}


@router.get(
    "/health",
    response_model=ApiEnvelope,
    summary="Health check",
)
def health() -> ApiEnvelope:
    payload = ApiEnvelope.ok()
    return payload


@router.get(
    "/types",
    response_model=ApiEnvelope,
    summary="List available export types",
)
def types() -> ApiEnvelope:
    payload = ApiEnvelope.ok(data={"items": TrtExporterManager.types()})
    return payload


@router.post(
    "/export",
    response_model=ApiEnvelope,
    summary="Export TensorRT engine",
    responses=EXPORT_RESPONSES,
)
def export(
    input: UploadFile = File(...),
    config: UploadFile = File(...),
) -> ApiEnvelope:
    payload = ExportTrtService().export(input, config)
    return payload
