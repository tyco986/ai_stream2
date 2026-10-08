import time

import httpx
from django.conf import settings


class HealthHttpClient:
    def __init__(self, timeout=None):
        self.timeout = (
            timeout
            if timeout is not None
            else settings.SERVERS_HEALTH_TIMEOUT
        )

    def get(self, url):
        started = time.monotonic()
        ok = False
        status_code = 0
        body = None
        detail = ""
        timed_out = False
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.get(url)
            status_code = response.status_code
            if response.content:
                try:
                    parsed = response.json()
                    if isinstance(parsed, dict):
                        body = parsed
                except ValueError:
                    body = None
            ok = 200 <= status_code < 300
            if ok and isinstance(body, dict) and "success" in body:
                ok = body.get("success") is True
            if not ok:
                detail = f"HTTP {status_code}"
                if isinstance(body, dict) and body.get("message"):
                    detail = str(body["message"])
        except httpx.TimeoutException as exc:
            timed_out = True
            detail = f"timeout: {exc}"
        except httpx.HTTPError as exc:
            detail = str(exc) or "connection error"
        latency_ms = int((time.monotonic() - started) * 1000)
        return {
            "ok": ok,
            "status_code": status_code,
            "body": body,
            "detail": detail,
            "latency_ms": latency_ms,
            "timed_out": timed_out,
        }
