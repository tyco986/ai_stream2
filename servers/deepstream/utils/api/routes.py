from fastapi import APIRouter, File, UploadFile

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope, SchemaRequest
from utils.api.services import PipelineService
from utils.manager.pipeline_manager import PipelineManager

router = APIRouter(prefix=f"/{PROJECT_NAME}/deepstream", tags=["deepstream"])


@router.get("/health", response_model=ApiEnvelope, summary="Health check")
def health() -> ApiEnvelope:
    payload = ApiEnvelope.ok()
    return payload


@router.get(
    "/pipeline/status",
    response_model=ApiEnvelope,
    summary="Pipeline running status",
)
def pipeline_status() -> ApiEnvelope:
    payload = ApiEnvelope.ok(data=PipelineManager.status())
    return payload


@router.get(
    "/types",
    response_model=ApiEnvelope,
    summary="List supported pipeline types",
)
def types() -> ApiEnvelope:
    payload = ApiEnvelope.ok(data={"items": PipelineManager.types()})
    return payload


@router.post(
    "/schema",
    response_model=ApiEnvelope,
    summary="Get pipeline probe-params schema",
)
def schema(body: SchemaRequest) -> ApiEnvelope:
    payload = PipelineService().schema(body.pipeline_type)
    return payload


@router.post(
    "/start_pipeline",
    response_model=ApiEnvelope,
    summary="Build and start a DeepStream pipeline",
)
def start_pipeline(input: UploadFile = File(...)) -> ApiEnvelope:
    payload = PipelineService().start(input)
    return payload
