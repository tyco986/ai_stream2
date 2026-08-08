from fastapi import APIRouter, Depends, File, UploadFile
from starlette.requests import Request

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope, SchemaRequest
from utils.api.services import PipelineStartService, SchemaService

router = APIRouter(prefix=f"/{PROJECT_NAME}/deepstream", tags=["deepstream"])


def get_pipeline_service(request: Request) -> PipelineStartService:
    return request.app.state.pipeline_service


def get_schema_service(request: Request) -> SchemaService:
    return request.app.state.schema_service


@router.get("/health", response_model=ApiEnvelope, summary="Health check")
def health() -> ApiEnvelope:
    return ApiEnvelope.ok()


@router.get(
    "/pipeline/status",
    response_model=ApiEnvelope,
    summary="Pipeline running status",
)
def route_pipeline_status(
    svc: PipelineStartService = Depends(get_pipeline_service),
) -> ApiEnvelope:
    return svc.get_status()


@router.get(
    "/types",
    response_model=ApiEnvelope,
    summary="List supported pipeline types",
)
def route_types(svc: PipelineStartService = Depends(get_pipeline_service)) -> ApiEnvelope:
    return svc.list_types()


@router.post(
    "/schema",
    response_model=ApiEnvelope,
    summary="Get pipeline probe-params schema",
)
def route_schema(
    body: SchemaRequest,
    svc: SchemaService = Depends(get_schema_service),
) -> ApiEnvelope:
    return svc.get_schema(body.pipeline_type)


@router.post(
    "/start_pipeline",
    response_model=ApiEnvelope,
    summary="Build and start a DeepStream pipeline",
)
def route_start_pipeline(
    input: UploadFile = File(...),
    svc: PipelineStartService = Depends(get_pipeline_service),
) -> ApiEnvelope:
    return svc.start(input)
