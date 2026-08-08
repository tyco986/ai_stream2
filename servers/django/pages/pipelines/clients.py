import httpx
from django.conf import settings

from shared.http.exceptions import AppError


class GeneratorClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.GENERATOR_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.PIPELINES_UPSTREAM_TIMEOUT
        )
        self.prefix = f"/{settings.PROJECT_NAME}/generator"

    def generate(self, yaml_bytes, filename="generator.yaml"):
        url = f"{self.base_url}{self.prefix}/generate"
        payload = {}
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.post(
                    url,
                    files={"input": (filename, yaml_bytes, "application/x-yaml")},
                )
        except httpx.HTTPError as exc:
            raise AppError(f"Generator unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if not isinstance(payload, dict):
            raise AppError("Invalid generator response", status_code=502)
        if response.status_code >= 500 or not payload.get("success"):
            message = payload.get("message") or f"Generator HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        return payload


class DeepStreamClient:
    def __init__(self, base_url, timeout=None):
        self.base_url = base_url.rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.PIPELINES_UPSTREAM_TIMEOUT
        )
        self.prefix = f"/{settings.PROJECT_NAME}/deepstream"

    def start_pipeline(self, yaml_bytes, filename="pipeline.yaml"):
        url = f"{self.base_url}{self.prefix}/start_pipeline"
        payload = {}
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.post(
                    url,
                    files={"input": (filename, yaml_bytes, "application/x-yaml")},
                )
        except httpx.HTTPError as exc:
            raise AppError(f"DeepStream unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if not isinstance(payload, dict):
            raise AppError("Invalid DeepStream response", status_code=502)
        if response.status_code >= 500 or not payload.get("success"):
            message = payload.get("message") or f"DeepStream HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        return payload

    def get_pipeline_status(self):
        url = f"{self.base_url}{self.prefix}/pipeline/status"
        payload = {}
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.get(url)
        except httpx.HTTPError as exc:
            raise AppError(f"DeepStream unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if not isinstance(payload, dict):
            raise AppError("Invalid DeepStream response", status_code=502)
        if response.status_code >= 500 or not payload.get("success"):
            message = payload.get("message") or f"DeepStream HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        data = payload.get("data")
        if not isinstance(data, dict):
            data = {}
        return data


class SnapshotClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.FFMPEG_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.FFMPEG_TIMEOUT_SECONDS
        )

    def capture(self, stream_id):
        raise AppError("Snapshot failed", status_code=502)
