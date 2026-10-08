from pathlib import Path

from django.conf import settings

from shared.clients.http import EnvelopeClient


class ExportOnnxClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.EXPORT_ONNX_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.MODELS_BUILD_TIMEOUT
        )
        self.http = EnvelopeClient(
            self.base_url,
            f"/{settings.PROJECT_NAME}/export_onnx",
            self.timeout,
            "export_onnx",
        )

    def health(self):
        return self.http.request("GET", "/health")

    def types(self):
        return self.http.request("GET", "/types")

    def export(self, input_path, config_yaml, filename=None):
        path = Path(input_path)
        name = filename or path.name
        payload = {}
        with path.open("rb") as handle:
            files = {
                "input": (name, handle, "application/octet-stream"),
                "config": ("config.yaml", config_yaml, "application/x-yaml"),
            }
            payload = self.http.request("POST", "/export", files=files)
        return payload
