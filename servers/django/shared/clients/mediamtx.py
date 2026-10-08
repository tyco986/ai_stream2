from urllib.parse import urlencode, urlparse

import httpx
from django.conf import settings

from shared.http.exceptions import AppError


class MediaMTXClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.MEDIAMTX_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.MEDIAMTX_TIMEOUT_SECONDS
        )
        self.record_root = settings.MEDIAMTX_RECORD_ROOT.rstrip("/")
        self.record_segment_duration = settings.MEDIAMTX_RECORD_SEGMENT_DURATION
        self.record_delete_after = settings.MEDIAMTX_RECORD_DELETE_AFTER
        self.playback_url = settings.MEDIAMTX_PLAYBACK_URL.rstrip("/")
        self.playback_timeout = settings.MEDIAMTX_PLAYBACK_TIMEOUT_SECONDS

    def upsert_path(self, name, source_url, record):
        source = "publisher" if self.is_self_publish(name, source_url) else source_url
        body = {"source": source, "record": bool(record)}
        if record:
            body["recordPath"] = f"{self.record_root}/%path/%Y-%m-%d_%H-%M-%S-%f"
            body["recordSegmentDuration"] = self.record_segment_duration
            body["recordDeleteAfter"] = self.record_delete_after
        add_status = self.request(
            "POST",
            f"/v3/config/paths/add/{name}",
            body,
            allow_statuses=(200, 400),
        )
        if add_status != 200:
            self.request(
                "POST",
                f"/v3/config/paths/replace/{name}",
                body,
                allow_statuses=(200,),
            )

    def is_self_publish(self, name, source_url):
        parsed = urlparse((source_url or "").strip())
        path_name = (parsed.path or "").strip("/")
        host = (parsed.hostname or "").lower()
        mediamtx_host = urlparse(self.base_url).hostname or ""
        known_hosts = {
            mediamtx_host.lower(),
            f"{settings.PROJECT_NAME}_mediamtx".lower(),
            "127.0.0.1",
            "localhost",
        }
        self_publish = False
        if path_name == name and host in known_hosts:
            self_publish = True
        return self_publish

    def delete_path(self, name):
        self.request(
            "DELETE",
            f"/v3/config/paths/delete/{name}",
            None,
            allow_statuses=(200, 404),
        )

    def get_json(self, method, path, allow_statuses):
        url = f"{self.base_url}{path}"
        status_code = 0
        body = None
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.request(method, url)
                status_code = response.status_code
                if response.content:
                    try:
                        parsed = response.json()
                        if isinstance(parsed, dict):
                            body = parsed
                    except ValueError:
                        body = None
        except httpx.HTTPError as exc:
            raise AppError(f"MediaMTX unreachable: {exc}", status_code=502) from exc
        if status_code not in allow_statuses:
            raise AppError(
                f"MediaMTX HTTP {status_code} for {method} {path}",
                status_code=502,
            )
        return {"status_code": status_code, "body": body}

    def inspect_path(self, name):
        enabled = False
        recording = False
        reachable = False
        detail = ""
        try:
            result = self.get_json(
                "GET",
                f"/v3/config/paths/get/{name}",
                allow_statuses=(200, 404),
            )
            reachable = True
            if result["status_code"] == 200 and isinstance(result["body"], dict):
                enabled = True
                recording = bool(result["body"].get("record"))
        except AppError as exc:
            detail = str(exc.detail)
        return {
            "reachable": reachable,
            "enabled": enabled,
            "recording": recording,
            "detail": detail,
        }

    def list_paths_index(self):
        paths = {}
        reachable = False
        detail = ""
        try:
            result = self.get_json(
                "GET",
                "/v3/config/paths/list",
                allow_statuses=(200,),
            )
            reachable = True
            body = result["body"] or {}
            items = body.get("items") or []
            for item in items:
                if not isinstance(item, dict):
                    continue
                path_name = item.get("name")
                if not path_name or path_name == "all_others":
                    continue
                paths[path_name] = {
                    "enabled": True,
                    "recording": bool(item.get("record")),
                }
        except AppError as exc:
            detail = str(exc.detail)
        return {"reachable": reachable, "paths": paths, "detail": detail}

    def list_playback(self, path, start, duration):
        items = []
        params = urlencode(
            {"path": path, "start": start, "duration": str(duration)}
        )
        url = f"{self.playback_url}/list?{params}"
        parsed = None
        try:
            with httpx.Client(timeout=self.playback_timeout) as client:
                response = client.get(url)
            if response.status_code == 200 and response.content:
                parsed = response.json()
        except (httpx.HTTPError, ValueError):
            parsed = None
        if isinstance(parsed, list):
            items = [item for item in parsed if isinstance(item, dict)]
        elif isinstance(parsed, dict):
            raw = parsed.get("items") or parsed.get("segments") or []
            if isinstance(raw, list):
                items = [item for item in raw if isinstance(item, dict)]
        return items

    def iter_playback(self, path, start, duration):
        params = urlencode(
            {
                "path": path,
                "start": start,
                "duration": str(duration),
                "format": "fmp4",
            }
        )
        url = f"{self.playback_url}/get?{params}"
        try:
            with httpx.Client(timeout=self.playback_timeout) as client:
                with client.stream("GET", url) as response:
                    if response.status_code != 200:
                        raise AppError(
                            f"MediaMTX HTTP {response.status_code} for GET /get",
                            status_code=502,
                        )
                    for chunk in response.iter_bytes():
                        if chunk:
                            yield chunk
        except httpx.HTTPError as exc:
            raise AppError(f"MediaMTX unreachable: {exc}", status_code=502) from exc

    def request(self, method, path, body, allow_statuses):
        url = f"{self.base_url}{path}"
        status_code = 0
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.request(method, url, json=body)
                status_code = response.status_code
        except httpx.HTTPError as exc:
            raise AppError(f"MediaMTX unreachable: {exc}", status_code=502) from exc
        if status_code not in allow_statuses:
            raise AppError(
                f"MediaMTX HTTP {status_code} for {method} {path}",
                status_code=502,
            )
        return status_code
