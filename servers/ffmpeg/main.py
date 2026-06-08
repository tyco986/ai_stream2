import argparse
import json
import logging
import os
import re
import shlex
import subprocess
import time
from collections.abc import Sequence
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.exceptions import RequestValidationError
from pydantic import BaseModel
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import JSONResponse, Response

TIMESTAMP_PATTERN = re.compile(r"^\d{2}:\d{2}:\d{2}(\.\d{1,3})?$")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
FFMPEG_BASE = ("ffmpeg", "-hide_banner", "-loglevel", "warning")
PUBLISHER_START_TIMEOUT_S = 1.0

_main_file = Path(__file__).resolve()
# Repo: servers/ffmpeg/main.py -> parents[2]; container: /app/main.py -> /app
PROJECT_ROOT = (
    _main_file.parents[2] if len(_main_file.parents) > 2 else _main_file.parent
)
HOST_OUTPUT_ROOT = PROJECT_ROOT / "outputs" / "ffmpeg"
HOST_LOG_ROOT = PROJECT_ROOT / "logs" / "ffmpeg"
HOST_VIDEO_ROOT = PROJECT_ROOT / "attachments" / "videos"
HOST_VIDEOS_ROOT = Path("/app/videos")


def _root_from_env(env_name: str, host_default: Path) -> Path:
    return Path(os.environ.get(env_name, host_default))


def configure_file_logger(log_root: Path, name: str = "ffmpeg_api") -> logging.Logger:
    log_root.mkdir(parents=True, exist_ok=True)
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    logger.handlers.clear()
    logger.propagate = False
    handler = RotatingFileHandler(
        log_root / "app.log",
        maxBytes=1024 * 1024,
        backupCount=10,
        encoding="utf-8",
    )
    handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(handler)
    return logger


def _api_ok(**kwargs) -> dict:
    return {"success": True, "message": "", **kwargs}


def _api_error(status_code: int, message: str) -> JSONResponse:
    return JSONResponse(
        status_code=status_code,
        content={"success": False, "message": message},
    )


async def _handle_http_exception(_: Request, exc: HTTPException) -> JSONResponse:
    detail = exc.detail
    message = detail if isinstance(detail, str) else str(detail)
    return _api_error(exc.status_code, message)


async def _handle_validation_error(
    _: Request, exc: RequestValidationError
) -> JSONResponse:
    return _api_error(422, str(exc.errors()))


class Video2RtspBody(BaseModel):
    input: str
    rtsp: str | None = None
    loop: bool = True


class Video2RtspStopBody(BaseModel):
    rtsp: str


class FrameExtractBody(BaseModel):
    input: str
    timestamp: str
    output: str | None = None


class RemoveBFrameBody(BaseModel):
    input: str
    output: str | None = None


class RtspInfoBody(BaseModel):
    rtsp: str


class ApiErrorResponse(BaseModel):
    success: bool = False
    message: str


class ApiOkResponse(BaseModel):
    success: bool = True
    message: str = ""


class HelloWorldResponse(ApiOkResponse):
    service: str


class PublisherItem(BaseModel):
    input: str
    rtsp: str
    pid: int
    loop: bool


class Video2RtspResponse(ApiOkResponse):
    input: str
    rtsp: str
    pid: int
    loop: bool
    command: str


class Video2RtspListResponse(ApiOkResponse):
    publishers: list[PublisherItem]


class Video2RtspStopResponse(ApiOkResponse):
    stopped: list[PublisherItem]


class FrameExtractResponse(ApiOkResponse):
    input: str
    timestamp: str
    output: str
    command: str


class RemoveBFrameResponse(ApiOkResponse):
    input: str
    output: str
    command: str


class RtspInfoResponse(ApiOkResponse):
    rtsp: str
    probe: dict
    command: str


_API_ERRORS = {
    400: {"model": ApiErrorResponse, "description": "Bad request"},
    404: {"model": ApiErrorResponse, "description": "Not found"},
    409: {"model": ApiErrorResponse, "description": "Conflict"},
    422: {"model": ApiErrorResponse, "description": "Validation error"},
    500: {"model": ApiErrorResponse, "description": "Server error"},
}


class RequestLogMiddleware(BaseHTTPMiddleware):
    def __init__(self, app, logger: logging.Logger) -> None:
        super().__init__(app)
        self._log = logger

    async def dispatch(self, request: Request, call_next) -> Response:
        start = time.perf_counter()
        method = request.method
        path = request.url.path
        client = request.client.host if request.client else "-"
        self._log.info("request start %s %s client=%s", method, path, client)
        try:
            response = await call_next(request)
        except Exception:
            elapsed_ms = (time.perf_counter() - start) * 1000
            self._log.exception(
                "request %s %s -> unhandled error (%.1f ms)",
                method,
                path,
                elapsed_ms,
            )
            raise

        elapsed_ms = (time.perf_counter() - start) * 1000
        log_msg = "request %s %s -> %s (%.1f ms)"
        log_args = (method, path, response.status_code, elapsed_ms)
        if response.status_code >= 400:
            self._log.warning(log_msg, *log_args)
        else:
            self._log.info(log_msg, *log_args)
        return response


class RtspPublisher:
    def __init__(
        self,
        process: subprocess.Popen[str],
        input_path: Path,
        rtsp_url: str,
        loop: bool,
    ) -> None:
        self.process = process
        self.input_path = input_path
        self.rtsp_url = rtsp_url
        self.loop = loop

    def is_running(self) -> bool:
        return self.process.poll() is None

    def to_dict(self) -> dict:
        return {
            "input": str(self.input_path),
            "rtsp": self.rtsp_url,
            "pid": self.process.pid,
            "loop": self.loop,
        }


class FFmpegServer:
    def __init__(
        self,
        output_root: Path | None = None,
        log_root: Path | None = None,
        video_root: Path | None = None,
        mediamtx_host: str | None = None,
        mediamtx_rtsp_port: int | None = None,
    ) -> None:
        self._project_root = PROJECT_ROOT
        self._output_root = output_root or _root_from_env(
            "OUTPUT_ROOT", HOST_OUTPUT_ROOT
        )
        self._video_root = video_root or _root_from_env("VIDEO_ROOT", HOST_VIDEO_ROOT)
        self._videos_root = _root_from_env("VIDEOS_ROOT", HOST_VIDEOS_ROOT)
        log_root_path = log_root or _root_from_env("LOG_ROOT", HOST_LOG_ROOT)
        self._logger = configure_file_logger(log_root_path)
        self._output_root.mkdir(parents=True, exist_ok=True)
        self._video_root.mkdir(parents=True, exist_ok=True)
        self._videos_root.mkdir(parents=True, exist_ok=True)
        self._input_roots = (
            self._video_root.resolve(),
            self._videos_root.resolve(),
            self._output_root.resolve(),
        )
        self._output_roots = (self._output_root.resolve(),)
        self._mediamtx_host = mediamtx_host or os.environ.get(
            "MEDIAMTX_HOST", "127.0.0.1"
        )
        self._mediamtx_rtsp_port = mediamtx_rtsp_port or int(
            os.environ.get("MEDIAMTX_RTSP_PORT", "8554")
        )
        self._publishers: dict[str, RtspPublisher] = {}

        self.app = FastAPI(
            title="FFmpeg API",
            description="HTTP wrapper for ffmpeg (RTSP publish, frame extract, B-frame removal).",
            version="1.0.0",
        )
        self.app.add_middleware(RequestLogMiddleware, logger=self._logger)
        self.app.add_exception_handler(HTTPException, _handle_http_exception)
        self.app.add_exception_handler(
            RequestValidationError, _handle_validation_error
        )
        self._register_routes()
        self._logger.info(
            "FFmpeg API service initialized log_root=%s", log_root_path
        )

    def _register_routes(self) -> None:
        self.app.add_api_route(
            "/ffmpeg/hello_world",
            self.hello_world,
            methods=["GET"],
            summary="Health check",
            response_model=HelloWorldResponse,
        )
        self.app.add_api_route(
            "/ffmpeg/video2rtsp",
            self.video2rtsp,
            methods=["POST"],
            summary="Publish video as RTSP",
            response_model=Video2RtspResponse,
            responses={k: _API_ERRORS[k] for k in (400, 404, 409, 500)},
        )
        self.app.add_api_route(
            "/ffmpeg/video2rtsp_list",
            self.video2rtsp_list,
            methods=["GET"],
            summary="List active RTSP publishers",
            response_model=Video2RtspListResponse,
        )
        self.app.add_api_route(
            "/ffmpeg/video2rtsp_stop",
            self.video2rtsp_stop,
            methods=["POST"],
            summary="Stop RTSP publisher(s)",
            response_model=Video2RtspStopResponse,
            responses={404: _API_ERRORS[404]},
        )
        self.app.add_api_route(
            "/ffmpeg/frame_extract",
            self.frame_extract,
            methods=["POST"],
            summary="Extract a single frame",
            response_model=FrameExtractResponse,
            responses={k: _API_ERRORS[k] for k in (400, 404, 422, 500)},
        )
        self.app.add_api_route(
            "/ffmpeg/remove_B_frame",
            self.remove_b_frame,
            methods=["POST"],
            summary="Remove B-frames from video",
            response_model=RemoveBFrameResponse,
            responses={k: _API_ERRORS[k] for k in (400, 404, 500)},
        )
        self.app.add_api_route(
            "/ffmpeg/rtsp_info",
            self.rtsp_info,
            methods=["POST"],
            summary="Probe RTSP stream with ffprobe",
            response_model=RtspInfoResponse,
            responses={k: _API_ERRORS[k] for k in (400, 500)},
        )

    def hello_world(self) -> dict:
        return _api_ok(service="ffmpeg")

    def video2rtsp(self, body: Video2RtspBody) -> dict:
        return self._video2rtsp(Path(body.input), body.rtsp, body.loop)

    def video2rtsp_list(self) -> dict:
        self._cleanup_publishers()
        return _api_ok(
            publishers=[p.to_dict() for p in self._publishers.values()],
        )

    def video2rtsp_stop(self, body: Video2RtspStopBody) -> dict:
        self._cleanup_publishers()
        if body.rtsp == "all":
            stopped = [self._stop_publisher(p) for p in list(self._publishers.values())]
            self._publishers.clear()
            return _api_ok(stopped=stopped)

        publisher = self._publishers.get(body.rtsp)
        if publisher is None or not publisher.is_running():
            raise HTTPException(
                status_code=404,
                detail=f"RTSP URL not publishing: {body.rtsp}",
            )
        stopped = self._stop_publisher(publisher)
        del self._publishers[body.rtsp]
        return _api_ok(stopped=[stopped])

    def frame_extract(self, body: FrameExtractBody) -> dict:
        output = Path(body.output) if body.output else None
        return self._invoke_subprocess(
            self._frame_extract,
            Path(body.input),
            body.timestamp,
            output,
        )

    def remove_b_frame(self, body: RemoveBFrameBody) -> dict:
        output = Path(body.output) if body.output else None
        return self._invoke_subprocess(
            self._remove_b_frame,
            Path(body.input),
            output,
        )

    def rtsp_info(self, body: RtspInfoBody) -> dict:
        return self._invoke_subprocess(self._rtsp_info, body.rtsp)

    def _video2rtsp(
        self,
        input_path: Path,
        rtsp_url: str | None,
        loop: bool,
    ) -> dict:
        resolved_input = self._resolve_input(input_path)
        target_rtsp = rtsp_url or self._default_rtsp_url(resolved_input)
        self._ensure_rtsp_available(target_rtsp)

        cmd = [*FFMPEG_BASE, "-re"]
        if loop:
            cmd.extend(["-stream_loop", "-1"])
        cmd.extend(
            [
                "-i",
                str(resolved_input),
                "-c",
                "copy",
                "-f",
                "rtsp",
                "-rtsp_transport",
                "tcp",
                target_rtsp,
            ]
        )
        process = subprocess.Popen(cmd, stderr=subprocess.PIPE, text=True)
        self._ensure_publisher_started(process, cmd)
        self._publishers[target_rtsp] = RtspPublisher(
            process, resolved_input, target_rtsp, loop
        )
        self._logger.info(
            "video2rtsp started pid=%s rtsp=%s input=%s loop=%s",
            process.pid,
            target_rtsp,
            resolved_input,
            loop,
        )
        return _api_ok(
            input=str(resolved_input),
            rtsp=target_rtsp,
            pid=process.pid,
            loop=loop,
            command=shlex.join(cmd),
        )

    def _frame_extract(
        self,
        input_path: Path,
        timestamp: str,
        output_path: Path | None,
    ) -> dict:
        resolved_input = self._resolve_input(input_path)
        normalized_ts = self._normalize_timestamp(timestamp)
        resolved_output = (
            self._resolve_output_path(output_path)
            if output_path is not None
            else self._default_frame_output(resolved_input, normalized_ts)
        )
        resolved_output.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            *FFMPEG_BASE,
            "-ss",
            normalized_ts,
            "-i",
            str(resolved_input),
            "-frames:v",
            "1",
            "-q:v",
            "2",
            str(resolved_output),
        ]
        self._run(cmd)
        return _api_ok(
            input=str(resolved_input),
            timestamp=normalized_ts,
            output=str(resolved_output),
            command=shlex.join(cmd),
        )

    def _remove_b_frame(self, input_path: Path, output_path: Path | None) -> dict:
        resolved_input = self._resolve_input(input_path)
        resolved_output = (
            self._resolve_output_path(output_path)
            if output_path is not None
            else self._default_remove_b_output(resolved_input)
        )
        resolved_output.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            *FFMPEG_BASE,
            "-i",
            str(resolved_input),
            "-c:v",
            "libx264",
            "-bf",
            "0",
            "-preset",
            "fast",
            "-c:a",
            "copy",
            str(resolved_output),
        ]
        self._run(cmd)
        return _api_ok(
            input=str(resolved_input),
            output=str(resolved_output),
            command=shlex.join(cmd),
        )

    def _invoke_subprocess(self, fn, *args, **kwargs) -> dict:
        try:
            return fn(*args, **kwargs)
        except subprocess.CalledProcessError as exc:
            detail = (exc.stderr or exc.stdout or str(exc)).strip()
            self._raise_ffmpeg_failure(list(exc.cmd), detail)

    def _ensure_publisher_started(
        self, process: subprocess.Popen[str], cmd: list[str]
    ) -> None:
        try:
            process.wait(timeout=PUBLISHER_START_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            return
        stderr = process.stderr.read() if process.stderr else ""
        detail = stderr.strip() or f"ffmpeg exited with code {process.returncode}"
        self._raise_ffmpeg_failure(cmd, detail)

    def _raise_ffmpeg_failure(self, cmd: list[str], detail: str) -> None:
        self._logger.error(
            "ffmpeg command failed: %s\nstderr:\n%s",
            cmd,
            detail,
        )
        raise HTTPException(
            status_code=500,
            detail=detail or "ffmpeg command failed",
        )

    def _run(self, cmd: list[str]) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            cmd,
            check=True,
            capture_output=True,
            text=True,
        )

    def _rtsp_info(self, rtsp_url: str) -> dict:
        if not rtsp_url.startswith("rtsp://"):
            raise HTTPException(
                status_code=400,
                detail="rtsp must start with rtsp://",
            )
        cmd = [
            "ffprobe",
            "-v",
            "quiet",
            "-rtsp_transport",
            "tcp",
            "-print_format",
            "json",
            "-show_format",
            "-show_streams",
            rtsp_url,
        ]
        result = self._run(cmd)
        return _api_ok(
            rtsp=rtsp_url,
            probe=json.loads(result.stdout),
            command=shlex.join(cmd),
        )

    def _default_rtsp_url(self, input_path: Path) -> str:
        return (
            f"rtsp://{self._mediamtx_host}:{self._mediamtx_rtsp_port}"
            f"/{input_path.stem}"
        )

    def _default_frame_output(self, input_path: Path, timestamp: str) -> Path:
        safe_ts = timestamp.replace(":", "-").replace(".", "-")
        return (
            self._output_root
            / "frame_extract"
            / f"{input_path.stem}_{safe_ts}.png"
        )

    def _default_remove_b_output(self, input_path: Path) -> Path:
        return (
            self._output_root / "remove_B_frame" / f"{input_path.stem}_B0.mp4"
        )

    def _storage_aliases(self) -> tuple[tuple[str, Path, bool], ...]:
        return (
            ("/app/video/", self._video_root, False),
            ("/app/videos/", self._videos_root, False),
            ("/app/output/", self._output_root, False),
            ("attachments/videos/", self._project_root, True),
            ("outputs/ffmpeg/", self._output_root, False),
        )

    def _map_storage_path(self, path: Path) -> Path:
        raw = path.as_posix()
        for prefix, root, keep_prefix in self._storage_aliases():
            if raw.startswith(prefix):
                if keep_prefix:
                    return root / raw
                return root / raw.removeprefix(prefix).lstrip("/")
        return path

    def _resolve_path(self, path: Path, allowed_roots: Sequence[Path]) -> Path:
        resolved = self._map_storage_path(path).expanduser().resolve()
        if not any(resolved.is_relative_to(root) for root in allowed_roots):
            raise HTTPException(
                status_code=400,
                detail=f"Path not allowed: {resolved}",
            )
        return resolved

    def _resolve_input(self, path: Path) -> Path:
        resolved = self._resolve_path(path, self._input_roots)
        if not resolved.is_file():
            raise HTTPException(
                status_code=404,
                detail=f"Input file not found: {resolved}",
            )
        return resolved

    def _resolve_output_path(self, path: Path) -> Path:
        return self._resolve_path(path, self._output_roots)

    def _normalize_timestamp(self, timestamp: str) -> str:
        if not TIMESTAMP_PATTERN.match(timestamp):
            raise HTTPException(
                status_code=422,
                detail="timestamp must be HH:MM:SS or HH:MM:SS.mmm",
            )
        if "." not in timestamp:
            return f"{timestamp}.000"
        base, frac = timestamp.split(".", 1)
        return f"{base}.{frac.ljust(3, '0')[:3]}"

    def _ensure_rtsp_available(self, rtsp_url: str) -> None:
        self._cleanup_publishers()
        existing = self._publishers.get(rtsp_url)
        if existing is not None and existing.is_running():
            raise HTTPException(
                status_code=409,
                detail=f"RTSP URL already publishing: {rtsp_url}",
            )
        if existing is not None:
            del self._publishers[rtsp_url]

    def _cleanup_publishers(self) -> None:
        dead = [rtsp for rtsp, p in self._publishers.items() if not p.is_running()]
        for rtsp in dead:
            del self._publishers[rtsp]

    def _stop_publisher(self, publisher: RtspPublisher) -> dict:
        if publisher.is_running():
            publisher.process.terminate()
            try:
                publisher.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                publisher.process.kill()
                publisher.process.wait()
        return publisher.to_dict()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="FFmpeg API service")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", default="8080")
    parser.add_argument(
        "--output-root",
        default=str(_root_from_env("OUTPUT_ROOT", HOST_OUTPUT_ROOT)),
    )
    parser.add_argument(
        "--log-root",
        default=str(_root_from_env("LOG_ROOT", HOST_LOG_ROOT)),
    )
    parser.add_argument(
        "--video-root",
        default=str(_root_from_env("VIDEO_ROOT", HOST_VIDEO_ROOT)),
    )
    parser.add_argument(
        "--mediamtx-host",
        default=os.environ.get("MEDIAMTX_HOST", "127.0.0.1"),
    )
    parser.add_argument(
        "--mediamtx-rtsp-port",
        type=int,
        default=int(os.environ.get("MEDIAMTX_RTSP_PORT", "8554")),
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = FFmpegServer(
        output_root=Path(args.output_root),
        log_root=Path(args.log_root),
        video_root=Path(args.video_root),
        mediamtx_host=args.mediamtx_host,
        mediamtx_rtsp_port=args.mediamtx_rtsp_port,
    )
    uvicorn.run(server.app, host=args.host, port=int(args.port))
