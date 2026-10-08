import httpx
from django.conf import settings

from shared.http.exceptions import AppError


class DockerProxyClient:
    def __init__(self, docker_host=None, timeout=None):
        self.docker_host = (
            docker_host if docker_host is not None else settings.DOCKER_HOST
        )
        self.timeout = (
            timeout
            if timeout is not None
            else settings.SERVERS_DOCKER_TIMEOUT
        )
        self.base_url = self.to_http_base(self.docker_host)

    def to_http_base(self, docker_host):
        raw = (docker_host or "").strip()
        base = raw
        if raw.startswith("tcp://"):
            base = "http://" + raw[len("tcp://") :]
        elif not raw.startswith("http://") and not raw.startswith("https://"):
            base = f"http://{raw}"
        return base.rstrip("/")

    def restart(self, container_name):
        path = f"/containers/{container_name}/restart"
        self.request("POST", path, expect_empty=True)

    def start(self, container_name):
        path = f"/containers/{container_name}/start"
        self.request("POST", path, expect_empty=True)

    def stop(self, container_name):
        path = f"/containers/{container_name}/stop"
        self.request("POST", path, expect_empty=True)

    def remove(self, container_name, force=True):
        force_flag = "1" if force else "0"
        path = f"/containers/{container_name}?force={force_flag}"
        self.request("DELETE", path, expect_empty=True)

    def create_container(self, name, body):
        path = f"/containers/create?name={name}"
        result = self.request(
            "POST", path, expect_empty=False, json_body=body, as_json=True
        )
        return result

    def inspect(self, container_name):
        path = f"/containers/{container_name}/json"
        result = self.request("GET", path, expect_empty=False, as_json=True)
        return result

    def logs(self, container_name, tail=500):
        path = (
            f"/containers/{container_name}/logs"
            f"?stdout=1&stderr=1&timestamps=1&tail={int(tail)}"
        )
        content = self.request("GET", path, expect_empty=False)
        return content

    def request(self, method, path, expect_empty, json_body=None, as_json=False):
        url = f"{self.base_url}{path}"
        result = ""
        kwargs = {}
        if json_body is not None:
            kwargs["json"] = json_body
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.request(method, url, **kwargs)
        except httpx.TimeoutException as exc:
            raise AppError(
                f"Docker proxy timeout: {exc}",
                status_code=504,
            ) from exc
        except httpx.HTTPError as exc:
            raise AppError(
                f"Docker proxy unreachable: {exc}",
                status_code=502,
            ) from exc
        if response.status_code >= 400:
            message = response.text.strip() or f"Docker HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        if expect_empty:
            result = ""
        elif as_json:
            if response.content:
                result = response.json()
            else:
                result = {}
        else:
            result = self.decode_docker_logs(response.content)
        return result

    def decode_docker_logs(self, raw):
        if not raw:
            return ""
        if len(raw) >= 8 and raw[0] in (0, 1, 2):
            chunks = []
            offset = 0
            while offset + 8 <= len(raw):
                size = int.from_bytes(raw[offset + 4 : offset + 8], "big")
                start = offset + 8
                end = start + size
                if end > len(raw):
                    break
                chunks.append(raw[start:end].decode("utf-8", errors="replace"))
                offset = end
            return "".join(chunks)
        return raw.decode("utf-8", errors="replace")
