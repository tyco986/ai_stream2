from fastapi import APIRouter, File, UploadFile

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope
from utils.api.services import ExportOnnxService
from utils.manager.onnx_exporter_manager import OnnxExporterManager

router = APIRouter(prefix=f"/{PROJECT_NAME}/export_onnx", tags=["export_onnx"])

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
    payload = ApiEnvelope.ok(data={"items": OnnxExporterManager.types()})
    return payload


@router.post(
    "/export",
    response_model=ApiEnvelope,
    summary="Export ONNX",
    responses=EXPORT_RESPONSES,
)
def export(
    input: UploadFile = File(...),
    config: UploadFile = File(...),
) -> ApiEnvelope:
    payload = ExportOnnxService().export(input, config)
    return payload
