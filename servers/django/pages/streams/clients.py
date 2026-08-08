from pathlib import Path
from urllib.parse import urlparse

import httpx
from django.conf import settings

from shared.http.exceptions import AppError


class FFmpegClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.FFMPEG_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.FFMPEG_TIMEOUT_SECONDS
        )
        self.prefix = f"/{settings.PROJECT_NAME}/ffmpeg"

    def probe(self, rtsp_url):
        payload = self.post_envelope(f"{self.prefix}/rtsp/probe", {"rtsp": rtsp_url})
        result = {
            "success": False,
            "error": payload.get("message") or "probe failed",
            "resolution": None,
            "fps": None,
        }
        if payload.get("success"):
            data = payload.get("data") or {}
            result = {
                "success": True,
                "error": None,
                "resolution": data.get("resolution"),
                "fps": data.get("fps"),
            }
        return result

    def batch_probe(self, rtsp_urls):
        payload = self.post_envelope(
            f"{self.prefix}/rtsp/batch/probe",
            {"rtsps": list(rtsp_urls)},
        )
        items = []
        if payload.get("success") and isinstance(payload.get("data"), list):
            items = payload["data"]
        return items

    def publish(self, upload, name=None):
        filename = Path(getattr(upload, "name", None) or "input").name
        content_type = (
            getattr(upload, "content_type", None) or "application/octet-stream"
        )
        form = {"loop": "true"}
        if name:
            form["name"] = name
        files = {"input": (filename, upload, content_type)}
        payload = self.post_multipart(
            f"{self.prefix}/rtsp/publishers",
            form,
            files,
            timeout=max(self.timeout, 300.0),
        )
        mapping = {}
        if payload.get("success") and isinstance(payload.get("data"), dict):
            mapping = payload["data"]
        if not mapping:
            message = payload.get("message") or "publish failed"
            raise AppError(message, status_code=502)
        stream_name, rtsp_url = next(iter(mapping.items()))
        result = {"name": stream_name, "url": rtsp_url}
        return result

    def post_envelope(self, path, body):
        url = f"{self.base_url}{path}"
        payload = {}
        try:
            with httpx.Client(timeout=self.timeout) as client:
                response = client.post(url, json=body)
        except httpx.HTTPError as exc:
            raise AppError(f"FFmpeg unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if response.status_code >= 500:
            message = payload.get("message") or f"FFmpeg HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        if not isinstance(payload, dict):
            raise AppError("Invalid FFmpeg response", status_code=502)
        return payload

    def post_multipart(self, path, form, files, timeout=None):
        url = f"{self.base_url}{path}"
        request_timeout = self.timeout if timeout is None else timeout
        payload = {}
        try:
            with httpx.Client(timeout=request_timeout) as client:
                response = client.post(url, data=form, files=files)
        except httpx.HTTPError as exc:
            raise AppError(f"FFmpeg unreachable: {exc}", status_code=502) from exc
        if response.content:
            payload = response.json()
        if response.status_code >= 500:
            message = payload.get("message") or f"FFmpeg HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        if not isinstance(payload, dict):
            raise AppError("Invalid FFmpeg response", status_code=502)
        if response.status_code >= 400 or not payload.get("success"):
            message = payload.get("message") or f"FFmpeg HTTP {response.status_code}"
            raise AppError(message, status_code=502)
        return payload


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
