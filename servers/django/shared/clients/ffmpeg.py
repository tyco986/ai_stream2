from pathlib import Path

from django.conf import settings

from shared.clients.http import EnvelopeClient


class FFmpegClient:
    def __init__(self, base_url=None, timeout=None):
        self.base_url = (
            base_url if base_url is not None else settings.FFMPEG_BASE_URL
        ).rstrip("/")
        self.timeout = (
            timeout if timeout is not None else settings.FFMPEG_TIMEOUT_SECONDS
        )
        self.http = EnvelopeClient(
            self.base_url,
            f"/{settings.PROJECT_NAME}/ffmpeg",
            self.timeout,
            "FFmpeg",
        )

    def health(self):
        return self.http.request("GET", "/health")

    def create_publisher(
        self,
        upload,
        name=None,
        loop=True,
        mediamtx_host=None,
        mediamtx_port=None,
    ):
        filename = Path(getattr(upload, "name", None) or "input").name
        content_type = (
            getattr(upload, "content_type", None) or "application/octet-stream"
        )
        form = {"loop": "true" if loop else "false"}
        if name:
            form["name"] = name
        if mediamtx_host:
            form["mediamtx_host"] = mediamtx_host
        if mediamtx_port is not None:
            form["mediamtx_port"] = str(int(mediamtx_port))
        files = {"input": (filename, upload, content_type)}
        return self.http.request(
            "POST",
            "/rtsp/publishers",
            data=form,
            files=files,
            timeout=max(self.timeout, 300.0),
        )

    def list_publishers(self):
        return self.http.request("GET", "/rtsp/publishers")

    def delete_publishers(self):
        return self.http.request("DELETE", "/rtsp/publishers")

    def delete_publisher(self, name):
        return self.http.request("DELETE", f"/rtsp/publishers/{name}")

    def probe_rtsp(self, rtsp):
        return self.http.request("POST", "/rtsp/probe", json_body={"rtsp": rtsp})

    def batch_probe_rtsp(self, rtsps):
        return self.http.request(
            "POST",
            "/rtsp/batch/probe",
            json_body={"rtsps": list(rtsps)},
        )

    def capture(self, input_path, timestamp=""):
        return self.http.request(
            "POST",
            "/video/capture",
            data={"input": input_path, "timestamp": timestamp},
        )

    def extract(self, upload, interval=1):
        filename = Path(getattr(upload, "name", None) or "input").name
        content_type = (
            getattr(upload, "content_type", None) or "application/octet-stream"
        )
        files = {"input": (filename, upload, content_type)}
        return self.http.request(
            "POST",
            "/video/extract",
            data={"interval": str(int(interval))},
            files=files,
        )

    def nob(self, upload):
        filename = Path(getattr(upload, "name", None) or "input").name
        content_type = (
            getattr(upload, "content_type", None) or "application/octet-stream"
        )
        files = {"input": (filename, upload, content_type)}
        return self.http.request(
            "POST",
            "/video/nob",
            files=files,
            envelope=False,
        )
