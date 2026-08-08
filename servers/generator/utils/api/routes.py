from fastapi import APIRouter, Depends, File, UploadFile
from starlette.requests import Request

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope, SchemaRequest
from utils.api.services import GenerateService, SchemaService

router = APIRouter(prefix=f"/{PROJECT_NAME}/generator", tags=["generator"])


def get_generate(request: Request) -> GenerateService:
    return request.app.state.generate_service


def get_schema_service(request: Request) -> SchemaService:
    return request.app.state.schema_service


@router.get("/health", response_model=ApiEnvelope, summary="Health check")
def health() -> ApiEnvelope:
    return ApiEnvelope.ok()


@router.get(
    "/types",
    response_model=ApiEnvelope,
    summary="List supported generator types",
)
def route_types(svc: GenerateService = Depends(get_generate)) -> ApiEnvelope:
    return svc.list_types()


@router.post(
    "/schema",
    response_model=ApiEnvelope,
    summary="Get generator __init__ schema",
)
def route_schema(
    body: SchemaRequest,
    svc: SchemaService = Depends(get_schema_service),
) -> ApiEnvelope:
    return svc.get_schema(body.generator)


@router.post(
    "/generate",
    response_model=ApiEnvelope,
    summary="Generate DeepStream pipeline configs",
)
def route_generate(
    input: UploadFile = File(...),
    svc: GenerateService = Depends(get_generate),
) -> ApiEnvelope:
    return svc.generate(input)
