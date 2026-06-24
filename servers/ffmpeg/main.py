import argparse
import json
import logging
import os
import re
import shlex
import shutil
import subprocess
import time
import traceback
from collections.abc import Callable
from logging.handlers import RotatingFileHandler
from pathlib import Path

import uvicorn
from fastapi import FastAPI, File, Form, UploadFile
from fastapi.exceptions import RequestValidationError
from fastapi.responses import FileResponse, JSONResponse
from pydantic import BaseModel
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response

PROJECT_NAME = "ai_stream2"
TIMESTAMP_PATTERN = re.compile(r"^\d{2}:\d{2}:\d{2}(\.\d{1,3})?$")
LOG_FORMAT = "[%(asctime)s] %(levelname)s %(name)s: %(message)s"
FFMPEG_BASE = ("ffmpeg", "-hide_banner", "-loglevel", "warning")
PUBLISHER_START_TIMEOUT_S = 1.0
DEFAULT_RECORDINGS_ROOT = Path("/root/recordings")
INPUT_ROOT = Path("/root/tmp")
FRAME_EXTRACT_OUTPUT_ROOT = Path("/root/outputs/ffmpeg/frame_extract")
REMOVE_B_FRAME_OUTPUT_ROOT = Path("/root/outputs/remove_B_frame")
DEFAULT_LOG_ROOT = Path(os.environ.get("LOG_ROOT", "/root/logs/ffmpeg"))
DEFAULT_HOST = os.environ.get("HOST", "0.0.0.0")
DEFAULT_PORT = int(os.environ.get("PORT", "8080"))


class ApiErrorResponse(BaseModel):
    success: bool = False
    message: str
    output: None = None
    command: str = ""


class ApiJsonResponse(BaseModel):
    success: bool = True
    message: str | dict | list = ""
    output: None = None
    command: str = ""


class RtspBody(BaseModel):
    rtsp: str


class RequestLogMiddleware(BaseHTTPMiddleware):
    def __init__(self, app, logger: logging.Logger) -> None:
        super().__init__(app)
        self.log = logger

    async def dispatch(self, request: Request, call_next) -> Response:
        start = time.perf_counter()
        method, path = request.method, request.url.path
        self.log.info("request start %s %s", method, path)
        response = await call_next(request)
        elapsed_ms = (time.perf_counter() - start) * 1000
        self.log.info(
            "request %s %s -> %s (%.1f ms)",
            method,
            path,
            response.status_code,
            elapsed_ms,
        )
        return response


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


def error_response(message: str, status_code: int = 400) -> JSONResponse:
    return JSONResponse(
        status_code=status_code,
        content=ApiErrorResponse(message=message).model_dump(),
    )


async def handle_validation_error(
    request: Request, exc: RequestValidationError
) -> JSONResponse:
    return error_response(str(exc.errors()), status_code=422)


def json_ok(message: str | dict | list = "", command: str = "") -> dict:
    return ApiJsonResponse(message=message, command=command).model_dump()


class VideoPathResolver:
    def __init__(self, recordings_root: Path) -> None:
        self.recordings_root = recordings_root

    def resolve(self, input_path: str) -> Path:
        path = Path(input_path).expanduser()
        if not path.is_absolute():
            path = self.recordings_root / path
        path = path.resolve()
        if not path.is_file():
            raise ValueError(f"input not found: {path}")
        return path


class InputStorage:
    @staticmethod
    def ensure(upload: UploadFile) -> Path:
        filename = Path(upload.filename or "input").name
        dest = INPUT_ROOT / filename
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        return dest


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
    def __init__(self, log_root: Path) -> None:
        self.log_root = log_root
        self.logger = configure_file_logger(log_root)
        self.publishers: dict[str, RtspPublisher] = {}
        self.video_resolver = VideoPathResolver(DEFAULT_RECORDINGS_ROOT)
        INPUT_ROOT.mkdir(parents=True, exist_ok=True)
        FRAME_EXTRACT_OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
        REMOVE_B_FRAME_OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)

        self.app = FastAPI(
            title="FFmpeg API",
            description="HTTP wrapper for ffmpeg (RTSP publish, frame extract, B-frame removal).",
            version="1.0.0",
        )
        self.app.add_middleware(RequestLogMiddleware, logger=self.logger)
        self.app.add_exception_handler(
            RequestValidationError, handle_validation_error
        )
        self.register_routes()
        self.logger.info("FFmpeg API initialized log_root=%s", log_root)

    def register_routes(self) -> None:
        prefix = f"/{PROJECT_NAME}/ffmpeg"
        self.app.add_api_route(
            f"{prefix}/hello_world",
            self.hello_world,
            methods=["GET"],
            summary="Health check",
            response_model=ApiJsonResponse,
        )
        self.app.add_api_route(
            f"{prefix}/video2rtsp",
            self.video2rtsp,
            methods=["POST"],
            summary="Publish video as RTSP",
            response_model=ApiJsonResponse,
            responses={400: {"model": ApiErrorResponse}},
        )
        self.app.add_api_route(
            f"{prefix}/video2rtsp_list",
            self.video2rtsp_list,
            methods=["GET"],
            summary="List active RTSP publishers",
            response_model=ApiJsonResponse,
        )
        self.app.add_api_route(
            f"{prefix}/video2rtsp_stop",
            self.video2rtsp_stop,
            methods=["POST"],
            summary="Stop RTSP publisher(s)",
            response_model=ApiJsonResponse,
            responses={404: {"model": ApiErrorResponse}},
        )
        self.app.add_api_route(
            f"{prefix}/frame_extract",
            self.frame_extract,
            methods=["POST"],
            summary="Extract a single frame",
            response_model=ApiJsonResponse,
            responses={
                400: {"model": ApiErrorResponse},
                422: {"model": ApiErrorResponse},
                500: {"model": ApiErrorResponse},
            },
        )
        self.app.add_api_route(
            f"{prefix}/remove_B_frame",
            self.remove_b_frame,
            methods=["POST"],
            summary="Remove B-frames from video",
            responses={
                200: {"content": {"video/mp4": {}}},
                400: {"model": ApiErrorResponse},
                500: {"model": ApiErrorResponse},
            },
        )
        self.app.add_api_route(
            f"{prefix}/rtsp_info",
            self.rtsp_info,
            methods=["POST"],
            summary="Probe RTSP stream with ffprobe",
            response_model=ApiJsonResponse,
            responses={400: {"model": ApiErrorResponse}, 500: {"model": ApiErrorResponse}},
        )

    def handle(self, route: str, action: Callable[[], Response | dict]) -> Response:
        try:
            result = action()
            if isinstance(result, Response):
                return result
            return JSONResponse(result)
        except Exception:
            message = traceback.format_exc()
            self.logger.error("%s failed\n%s", route, message)
            return error_response(message)

    def hello_world(self) -> dict:
        return json_ok()

    def video2rtsp(
        self,
        input: UploadFile = File(...),
        rtsp: str | None = Form(None),
        loop: bool = Form(True),
        mediamtx_host: str = Form("ai_stream2_mediamtx"),
        mediamtx_port: int = Form(8554),
    ) -> Response:
        return self.handle("video2rtsp", lambda: self.run_video2rtsp(
            input, rtsp, loop, mediamtx_host, mediamtx_port
        ))

    def video2rtsp_list(self) -> Response:
        return self.handle("video2rtsp_list", self.run_video2rtsp_list)

    def run_video2rtsp_list(self) -> dict:
        self.cleanup_publishers()
        publishers = [publisher.to_dict() for publisher in self.publishers.values()]
        return json_ok(message=publishers)

    def video2rtsp_stop(self, body: RtspBody) -> Response:
        return self.handle("video2rtsp_stop", lambda: self.run_video2rtsp_stop(body))

    def frame_extract(
        self,
        input: str = Form(...),
        timestamp: str = Form(""),
    ) -> Response:
        return self.handle("frame_extract", lambda: self.run_frame_extract(input, timestamp))

    def remove_b_frame(self, input: UploadFile = File(...)) -> Response:
        return self.handle("remove_B_frame", lambda: self.run_remove_b_frame(input))

    def rtsp_info(self, body: RtspBody) -> Response:
        return self.handle("rtsp_info", lambda: self.run_rtsp_info(body))

    def run_video2rtsp(
        self,
        upload: UploadFile,
        rtsp: str | None,
        loop: bool,
        mediamtx_host: str,
        mediamtx_port: int,
    ) -> dict:
        input_path = InputStorage.ensure(upload)
        target_rtsp = rtsp or f"rtsp://{mediamtx_host}:{mediamtx_port}/{input_path.stem}"
        self.ensure_rtsp_available(target_rtsp)

        cmd = [*FFMPEG_BASE, "-re"]
        if loop:
            cmd.extend(["-stream_loop", "-1"])
        cmd.extend(
            [
                "-i",
                str(input_path),
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
        self.ensure_publisher_started(process)
        publisher = RtspPublisher(process, input_path, target_rtsp, loop)
        self.publishers[target_rtsp] = publisher
        return json_ok(message=publisher.to_dict(), command=shlex.join(cmd))

    def run_video2rtsp_stop(self, body: RtspBody) -> Response | dict:
        self.cleanup_publishers()
        if body.rtsp == "all":
            stopped = [
                self.stop_publisher(publisher)
                for publisher in list(self.publishers.values())
            ]
            self.publishers.clear()
            return json_ok(message=stopped)

        publisher = self.publishers.get(body.rtsp)
        if publisher is None or not publisher.is_running():
            return error_response(f"RTSP URL not publishing: {body.rtsp}", 404)
        stopped = self.stop_publisher(publisher)
        del self.publishers[body.rtsp]
        return json_ok(message=[stopped])

    def run_frame_extract(self, input_path: str, timestamp: str) -> dict:
        video_path = self.video_resolver.resolve(input_path)
        normalized_ts = self.normalize_timestamp(timestamp)
        safe_ts = normalized_ts.replace(":", "-").replace(".", "-")
        output_path = FRAME_EXTRACT_OUTPUT_ROOT / f"{video_path.stem}_{safe_ts}.png"
        output_path.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            *FFMPEG_BASE,
            "-ss",
            normalized_ts,
            "-i",
            str(video_path),
            "-frames:v",
            "1",
            "-q:v",
            "2",
            str(output_path),
        ]
        self.run_ffmpeg(cmd)
        return json_ok(message=str(output_path), command=shlex.join(cmd))

    def run_remove_b_frame(self, upload: UploadFile) -> FileResponse:
        input_path = InputStorage.ensure(upload)
        output_path = REMOVE_B_FRAME_OUTPUT_ROOT / f"{input_path.stem}_B0.mp4"
        output_path.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            *FFMPEG_BASE,
            "-i",
            str(input_path),
            "-c:v",
            "libx264",
            "-bf",
            "0",
            "-preset",
            "fast",
            "-c:a",
            "copy",
            str(output_path),
        ]
        self.run_ffmpeg(cmd)
        command = shlex.join(cmd)
        return FileResponse(
            output_path,
            media_type="video/mp4",
            filename=output_path.name,
            headers={"X-Command": command, "X-Message": ""},
        )

    def run_rtsp_info(self, body: RtspBody) -> dict:
        if not body.rtsp.startswith("rtsp://"):
            return error_response("rtsp must start with rtsp://", 400)
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
            body.rtsp,
        ]
        result = self.run_ffmpeg(cmd)
        probe_text = result.stdout.strip()
        probe = json.loads(probe_text) if probe_text else {}
        return json_ok(message=probe, command=shlex.join(cmd))

    def run_ffmpeg(self, cmd: list[str]) -> subprocess.CompletedProcess[str]:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            detail = (result.stderr or result.stdout or "ffmpeg command failed").strip()
            self.logger.error("ffmpeg failed cmd=%s detail=%s", cmd, detail)
            raise RuntimeError(detail)
        return result

    def ensure_publisher_started(self, process: subprocess.Popen[str]) -> None:
        try:
            process.wait(timeout=PUBLISHER_START_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            return
        stderr = process.stderr.read() if process.stderr else ""
        detail = stderr.strip() or f"ffmpeg exited with code {process.returncode}"
        raise RuntimeError(detail)

    def normalize_timestamp(self, timestamp: str) -> str:
        if not TIMESTAMP_PATTERN.match(timestamp):
            raise ValueError("timestamp must be HH:MM:SS or HH:MM:SS.mmm")
        if "." not in timestamp:
            return f"{timestamp}.000"
        base, frac = timestamp.split(".", 1)
        return f"{base}.{frac.ljust(3, '0')[:3]}"

    def ensure_rtsp_available(self, rtsp_url: str) -> None:
        self.cleanup_publishers()
        existing = self.publishers.get(rtsp_url)
        if existing is not None and existing.is_running():
            raise RuntimeError(f"RTSP URL already publishing: {rtsp_url}")
        if existing is not None:
            del self.publishers[rtsp_url]

    def cleanup_publishers(self) -> None:
        dead = [
            rtsp
            for rtsp, publisher in self.publishers.items()
            if not publisher.is_running()
        ]
        for rtsp in dead:
            del self.publishers[rtsp]

    def stop_publisher(self, publisher: RtspPublisher) -> dict:
        if not publisher.is_running():
            return publisher.to_dict()
        publisher.process.terminate()
        try:
            publisher.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            publisher.process.kill()
            publisher.process.wait()
        return publisher.to_dict()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="FFmpeg API service")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--log-root", default=str(DEFAULT_LOG_ROOT))
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    server = FFmpegServer(log_root=Path(args.log_root))
    uvicorn.run(server.app, host=args.host, port=args.port)
