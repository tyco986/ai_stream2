from django.conf import settings

from shared.clients.http import EnvelopeClient


class ExportTrtClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.EXPORT_TRT_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.MODELS_BUILD_TIMEOUT
        )
        self.http = EnvelopeClient(
            self.base_url,
            f"/{settings.PROJECT_NAME}/export_trt",
            self.timeout,
            "export_trt",
        )

    def health(self):
        return self.http.request("GET", "/health")

    def types(self):
        return self.http.request("GET", "/types")

    def export(self, zip_bytes, config_yaml, filename="onnx.zip"):
        files = {
            "input": (filename, zip_bytes, "application/zip"),
            "config": ("config.yaml", config_yaml, "application/x-yaml"),
        }
        return self.http.request("POST", "/export", files=files)

    def test(self, config_yaml, filename="test.yaml"):
        files = {"config": (filename, config_yaml, "application/x-yaml")}
        return self.http.request("POST", "/test", files=files)
