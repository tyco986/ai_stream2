from fastapi import APIRouter, Depends, File, Form, UploadFile
from fastapi.responses import FileResponse
from starlette.requests import Request

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope, RtspBatchBody, RtspBody
from utils.api.services import (
    CaptureService,
    NobService,
    RtspProbeService,
    Video2RtspService,
    default_mediamtx_host,
)

router = APIRouter(prefix=f"/{PROJECT_NAME}/ffmpeg", tags=["ffmpeg"])


def get_publishers(request: Request) -> Video2RtspService:
    return request.app.state.publisher_service


def get_rtsp_probe(request: Request) -> RtspProbeService:
    return request.app.state.rtsp_probe


def get_capture(request: Request) -> CaptureService:
    return request.app.state.capture


def get_nob(request: Request) -> NobService:
    return request.app.state.nob


@router.get("/health", response_model=ApiEnvelope, summary="Health check")
def health() -> ApiEnvelope:
    return ApiEnvelope.ok()


@router.post("/rtsp/publishers", response_model=ApiEnvelope, summary="Publish video as RTSP")
def route_publishers_create(
    input: UploadFile = File(...),
    name: str | None = Form(None),
    loop: bool = Form(True),
    mediamtx_host: str | None = Form(None),
    mediamtx_port: int = Form(8554),
    svc: Video2RtspService = Depends(get_publishers),
) -> ApiEnvelope:
    host = mediamtx_host or default_mediamtx_host()
    return svc.publish(input, name, loop, host, mediamtx_port)


@router.get("/rtsp/publishers", response_model=ApiEnvelope, summary="List RTSP publishers")
def route_publishers_list(
    svc: Video2RtspService = Depends(get_publishers),
) -> ApiEnvelope:
    return svc.list_active()


@router.delete("/rtsp/publishers", response_model=ApiEnvelope, summary="Stop all RTSP publishers")
def route_publishers_delete_all(
    svc: Video2RtspService = Depends(get_publishers),
) -> ApiEnvelope:
    return svc.stop_all()


@router.delete(
    "/rtsp/publishers/{name}",
    response_model=ApiEnvelope,
    summary="Stop one RTSP publisher",
)
def route_publishers_delete_one(
    name: str,
    svc: Video2RtspService = Depends(get_publishers),
) -> ApiEnvelope:
    return svc.stop_one(name)


@router.post("/rtsp/probe", response_model=ApiEnvelope, summary="Probe RTSP")
def route_rtsp_probe(
    body: RtspBody,
    svc: RtspProbeService = Depends(get_rtsp_probe),
) -> ApiEnvelope:
    return svc.probe_one(body)


@router.post("/rtsp/batch/probe", response_model=ApiEnvelope, summary="Batch probe RTSP")
def route_rtsp_batch_probe(
    body: RtspBatchBody,
    svc: RtspProbeService = Depends(get_rtsp_probe),
) -> ApiEnvelope:
    return svc.probe_batch(body)


@router.post("/video/capture", response_model=ApiEnvelope, summary="Capture frame")
def route_capture(
    input: str = Form(...),
    timestamp: str = Form(""),
    svc: CaptureService = Depends(get_capture),
) -> ApiEnvelope:
    return svc.capture(input, timestamp)


@router.post("/video/nob", summary="Encode video without B-frames")
def route_nob(
    input: UploadFile = File(...),
    svc: NobService = Depends(get_nob),
) -> FileResponse:
    return svc.encode(input)
