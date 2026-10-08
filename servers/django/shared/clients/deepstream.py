from django.conf import settings

from shared.clients.http import EnvelopeClient


class DeepStreamClient:
    def __init__(self, base_url, timeout=None):
        self.base_url = base_url.rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.PIPELINES_UPSTREAM_TIMEOUT
        )
        self.http = EnvelopeClient(
            self.base_url,
            f"/{settings.PROJECT_NAME}/deepstream",
            self.timeout,
            "DeepStream",
        )

    def health(self):
        return self.http.request("GET", "/health")

    def pipeline_status(self):
        return self.http.request("GET", "/pipeline/status")

    def start_pipeline(self, config_dir):
        return self.http.request(
            "POST",
            "/start_pipeline",
            json_body={"config_dir": str(config_dir)},
        )
