from django.conf import settings

from shared.clients.http import EnvelopeClient


class GeneratorClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.GENERATOR_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.PIPELINES_UPSTREAM_TIMEOUT
        )
        self.http = EnvelopeClient(
            self.base_url,
            f"/{settings.PROJECT_NAME}/generator",
            self.timeout,
            "Generator",
        )

    def health(self):
        return self.http.request("GET", "/health")

    def types(self):
        return self.http.request("GET", "/types")

    def schema(self, generator):
        return self.http.request(
            "POST",
            "/schema",
            json_body={"generator": generator},
        )

    def generate(self, yaml_bytes, filename="generator.yaml"):
        files = {"input": (filename, yaml_bytes, "application/x-yaml")}
        return self.http.request("POST", "/generate", files=files)
