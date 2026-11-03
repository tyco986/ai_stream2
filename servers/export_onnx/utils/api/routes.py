from pathlib import Path

from fastapi import APIRouter, File, Query, Request, UploadFile
from fastapi.responses import Response

from utils.api.constants import (
    FIELD_CONFIG,
    FIELD_FILENAME,
    FIELD_INPUT,
    PROJECT_NAME,
    STATE_INPUT,
)
from utils.api.response import ApiEnvelope
from utils.api.services import ExportOnnxService
from utils.manager.onnx_exporter_manager import OnnxExporterManager

router = APIRouter(prefix=f"/{PROJECT_NAME}/export_onnx", tags=["export_onnx"])


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


@router.get(
    "/history",
    response_model=ApiEnvelope,
    summary="List export history",
)
def history(filename: str = Query(default="", alias=FIELD_FILENAME)) -> ApiEnvelope:
    payload = ApiEnvelope.ok(data=ExportOnnxService().get_history(filename))
    return payload


@router.post(
    "/history",
    summary="Download a history zip",
)
def download_history(filename: str = Query(default="", alias=FIELD_FILENAME)) -> Response:
    payload = ExportOnnxService().download_history(filename)
    return payload


@router.delete(
    "/history",
    response_model=ApiEnvelope,
    summary="Remove export history zip files",
)
def remove_history(filename: str = Query(default="", alias=FIELD_FILENAME)) -> ApiEnvelope:
    ExportOnnxService().remove_history(filename)
    payload = ApiEnvelope.ok()
    return payload


@router.delete(
    "/logs",
    response_model=ApiEnvelope,
    summary="Remove request logs",
)
def remove_logs() -> ApiEnvelope:
    ExportOnnxService().remove_logs()
    payload = ApiEnvelope.ok()
    return payload


@router.post(
    "/export",
    summary="Export ONNX",
)
def export(
    request: Request,
    input: UploadFile = File(..., alias=FIELD_INPUT),
    config: UploadFile = File(..., alias=FIELD_CONFIG),
) -> Response:
    setattr(request.state, STATE_INPUT, Path(input.filename or "").name)
    payload = ExportOnnxService().export(input, config)
    return payload
