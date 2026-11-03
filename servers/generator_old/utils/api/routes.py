from fastapi import APIRouter, File, UploadFile

from utils.api.constants import PROJECT_NAME
from utils.api.schemas import ApiEnvelope, SchemaRequest
from utils.api.services import GeneratorService
from utils.manager.generator_manager import GeneratorManager

router = APIRouter(prefix=f"/{PROJECT_NAME}/generator", tags=["generator"])


@router.get("/health", response_model=ApiEnvelope, summary="Health check")
def health() -> ApiEnvelope:
    payload = ApiEnvelope.ok()
    return payload


@router.get(
    "/types",
    response_model=ApiEnvelope,
    summary="List supported generator types",
)
def types() -> ApiEnvelope:
    payload = ApiEnvelope.ok(data={"items": GeneratorManager.types()})
    return payload


@router.post(
    "/schema",
    response_model=ApiEnvelope,
    summary="Get generator __init__ schema",
)
def schema(body: SchemaRequest) -> ApiEnvelope:
    payload = GeneratorService().schema(body.generator)
    return payload


@router.post(
    "/generate",
    response_model=ApiEnvelope,
    summary="Generate DeepStream pipeline configs",
)
def generate(input: UploadFile = File(...)) -> ApiEnvelope:
    payload = GeneratorService().generate(input)
    return payload
