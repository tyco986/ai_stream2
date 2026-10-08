import httpx

from shared.http.exceptions import AppError


class EnvelopeClient:
    def __init__(self, base_url, prefix, timeout, name):
        self.base_url = (base_url or "").rstrip("/")
        self.prefix = prefix if str(prefix).startswith("/") else f"/{prefix}"
        self.timeout = timeout
        self.name = name

    def request(
        self,
        method,
        path,
        json_body=None,
        data=None,
        files=None,
        timeout=None,
        envelope=True,
    ):
        url = f"{self.base_url}{self.prefix}{path}"
        request_timeout = self.timeout if timeout is None else timeout
        kwargs = {}
        if json_body is not None:
            kwargs["json"] = json_body
        if data is not None:
            kwargs["data"] = data
        if files is not None:
            kwargs["files"] = files
        try:
            with httpx.Client(timeout=request_timeout) as client:
                response = client.request(method, url, **kwargs)
        except httpx.HTTPError as exc:
            raise AppError(f"{self.name} unreachable: {exc}", status_code=502) from exc
        payload = {}
        if envelope:
            if response.content:
                payload = response.json()
            if not isinstance(payload, dict):
                raise AppError(f"Invalid {self.name} response", status_code=502)
            failed = response.status_code >= 400 or not payload.get("success")
            if failed:
                message = (
                    payload.get("message")
                    or f"{self.name} HTTP {response.status_code}"
                )
                raise AppError(message, status_code=502)
            result = payload
        else:
            if response.status_code >= 400:
                raise AppError(
                    f"{self.name} HTTP {response.status_code}",
                    status_code=502,
                )
            result = response.content
        return result
