import json
import logging
import shlex
import shutil
import subprocess
from pathlib import Path

from fastapi import UploadFile
from fastapi.responses import FileResponse

from utils.api.constants import (
    CAPTURE_OUTPUT_ROOT,
    DEFAULT_RECORDINGS_ROOT,
    FFMPEG_BASE,
    FFPROBE_BASE,
    INPUT_ROOT,
    LOGGER_NAME,
    NOB_OUTPUT_ROOT,
    PROJECT_NAME,
    PUBLISHER_START_TIMEOUT_S,
    TIMESTAMP_PATTERN,
)
from utils.api.schemas import ApiEnvelope, RtspBatchBody, RtspBody


class AppError(Exception):
    def __init__(self, message: str, status_code: int = 400) -> None:
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class FFmpegRunner:
    def __init__(self) -> None:
        self.logger = logging.getLogger(LOGGER_NAME)

    def run(self, cmd: list[str]) -> subprocess.CompletedProcess[str]:
        result = self.run_soft(cmd)
        if result.returncode != 0:
            detail = (result.stderr or result.stdout or "ffmpeg command failed").strip()
            self.logger.error("ffmpeg failed cmd=%s detail=%s", cmd, detail)
            raise AppError(detail)
        return result

    def run_soft(self, cmd: list[str]) -> subprocess.CompletedProcess[str]:
        return subprocess.run(cmd, capture_output=True, text=True)

    def ensure_publisher_started(self, process: subprocess.Popen[str]) -> None:
        timed_out = False
        try:
            process.wait(timeout=PUBLISHER_START_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            timed_out = True
        if not timed_out:
            stderr = process.stderr.read() if process.stderr else ""
            detail = stderr.strip() or f"ffmpeg exited with code {process.returncode}"
            raise AppError(detail)


class VideoPathResolver:
    def __init__(self, recordings_root: Path = DEFAULT_RECORDINGS_ROOT) -> None:
        self.recordings_root = recordings_root

    def resolve(self, input_path: str) -> Path:
        path = Path(input_path).expanduser()
        if not path.is_absolute():
            path = self.recordings_root / path
        path = path.resolve()
        if not path.is_file():
            raise AppError(f"input not found: {path}", status_code=404)
        return path


class InputStorage:
    def __init__(self, root: Path = INPUT_ROOT) -> None:
        self.root = root

    def ensure(self, upload: UploadFile) -> Path:
        filename = Path(upload.filename or "input").name
        dest = self.root / filename
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open("wb") as handle:
            shutil.copyfileobj(upload.file, handle)
        return dest


class RtspPublisher:
    def __init__(
        self,
        process: subprocess.Popen[str],
        input_path: Path,
        name: str,
        rtsp_url: str,
        loop: bool,
    ) -> None:
        self.process = process
        self.input_path = input_path
        self.name = name
        self.rtsp_url = rtsp_url
        self.loop = loop

    def is_running(self) -> bool:
        return self.process.poll() is None


class Video2RtspService:
    def __init__(
        self,
        runner: FFmpegRunner,
        storage: InputStorage,
        publishers: dict[str, RtspPublisher],
    ) -> None:
        self.runner = runner
        self.storage = storage
        self.publishers = publishers

    def publish(
        self,
        upload: UploadFile,
        name: str | None,
        loop: bool,
        mediamtx_host: str,
        mediamtx_port: int,
    ) -> ApiEnvelope:
        input_path = self.storage.ensure(upload)
        stream_name = self.resolve_name(name, input_path)
        target_rtsp = f"rtsp://{mediamtx_host}:{mediamtx_port}/{stream_name}"
        self.ensure_name_available(stream_name)
        cmd = [*FFMPEG_BASE, "-re", "-fflags", "+genpts"]
        if loop:
            cmd.extend(["-stream_loop", "-1"])
        cmd.extend(
            [
                "-i",
                str(input_path),
                "-c:v",
                "copy",
                "-an",
                "-f",
                "rtsp",
                "-rtsp_transport",
                "tcp",
                target_rtsp,
            ]
        )
        process = subprocess.Popen(cmd, stderr=subprocess.PIPE, text=True)
        self.runner.ensure_publisher_started(process)
        publisher = RtspPublisher(process, input_path, stream_name, target_rtsp, loop)
        self.publishers[stream_name] = publisher
        return ApiEnvelope.ok(
            data={stream_name: target_rtsp}, command=shlex.join(cmd)
        )

    def list_active(self) -> ApiEnvelope:
        self.cleanup_publishers()
        mapping = {
            name: publisher.rtsp_url for name, publisher in self.publishers.items()
        }
        return ApiEnvelope.ok(data=mapping)

    def stop_all(self) -> ApiEnvelope:
        self.cleanup_publishers()
        mapping = {
            name: publisher.rtsp_url for name, publisher in self.publishers.items()
        }
        for publisher in list(self.publishers.values()):
            self.stop_publisher(publisher)
        self.publishers.clear()
        return ApiEnvelope.ok(data=mapping)

    def stop_one(self, name: str) -> ApiEnvelope:
        self.cleanup_publishers()
        publisher = self.publishers.get(name)
        if publisher is None or not publisher.is_running():
            raise AppError(f"publisher not found: {name}", status_code=404)
        self.stop_publisher(publisher)
        del self.publishers[name]
        return ApiEnvelope.ok(data={name: publisher.rtsp_url})

    def resolve_name(self, name: str | None, input_path: Path) -> str:
        stream_name = (name or input_path.stem).strip()
        if not stream_name or "/" in stream_name or "\\" in stream_name:
            raise AppError("name must be a non-empty path segment")
        return stream_name

    def ensure_name_available(self, name: str) -> None:
        self.cleanup_publishers()
        if name in self.publishers:
            raise AppError(f"publisher already exists: {name}")

    def cleanup_publishers(self) -> None:
        dead = [
            name
            for name, publisher in self.publishers.items()
            if not publisher.is_running()
        ]
        for name in dead:
            del self.publishers[name]

    def stop_publisher(self, publisher: RtspPublisher) -> None:
        if publisher.is_running():
            publisher.process.terminate()
            wait_ok = True
            try:
                publisher.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                wait_ok = False
            if not wait_ok:
                publisher.process.kill()
                publisher.process.wait()


class CaptureService:
    def __init__(self, runner: FFmpegRunner, resolver: VideoPathResolver) -> None:
        self.runner = runner
        self.resolver = resolver

    def capture(self, input_path: str, timestamp: str) -> ApiEnvelope:
        video_path = self.resolver.resolve(input_path)
        normalized_ts = self.normalize_timestamp(timestamp)
        safe_ts = normalized_ts.replace(":", "-").replace(".", "-")
        output_path = CAPTURE_OUTPUT_ROOT / f"{video_path.stem}_{safe_ts}.png"
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
        self.runner.run(cmd)
        return ApiEnvelope.ok(data=str(output_path), command=shlex.join(cmd))

    def normalize_timestamp(self, timestamp: str) -> str:
        if not TIMESTAMP_PATTERN.match(timestamp):
            raise AppError("timestamp must be HH:MM:SS or HH:MM:SS.mmm")
        normalized = timestamp
        if "." not in timestamp:
            normalized = f"{timestamp}.000"
        else:
            base, frac = timestamp.split(".", 1)
            normalized = f"{base}.{frac.ljust(3, '0')[:3]}"
        return normalized


class NobService:
    def __init__(self, runner: FFmpegRunner, storage: InputStorage) -> None:
        self.runner = runner
        self.storage = storage

    def encode(self, upload: UploadFile) -> FileResponse:
        input_path = self.storage.ensure(upload)
        output_path = NOB_OUTPUT_ROOT / f"{input_path.stem}_nob.mp4"
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
        self.runner.run(cmd)
        return FileResponse(
            output_path,
            media_type="video/mp4",
            filename=output_path.name,
            headers={"X-Command": shlex.join(cmd)},
        )


class ProbeSummaryMapper:
    """ffprobe JSON → {resolution, fps}."""

    def summarize(self, probe: dict) -> dict | None:
        video = self.pick_video_stream(probe)
        summary = None
        if video is not None:
            width = video.get("width")
            height = video.get("height")
            fps = self.parse_fps(video.get("avg_frame_rate") or video.get("r_frame_rate"))
            if width and height and fps is not None:
                summary = {"resolution": f"{width}x{height}", "fps": fps}
        return summary

    def pick_video_stream(self, probe: dict) -> dict | None:
        video = None
        for stream in probe.get("streams") or []:
            if stream.get("codec_type") == "video":
                video = stream
                break
        return video

    def parse_fps(self, rate: str | None) -> int | None:
        fps = None
        if rate and rate not in {"0/0", "N/A"}:
            if "/" in rate:
                num_s, den_s = rate.split("/", 1)
                num = float(num_s)
                den = float(den_s)
                if den != 0:
                    fps = int(round(num / den))
            else:
                fps = int(round(float(rate)))
        return fps


class RtspProbeService:
    def __init__(self, runner: FFmpegRunner) -> None:
        self.runner = runner
        self.mapper = ProbeSummaryMapper()

    def probe_one(self, body: RtspBody) -> ApiEnvelope:
        if not body.rtsp.startswith("rtsp://"):
            raise AppError("rtsp must start with rtsp://")
        return self.probe_response(body.rtsp)

    def probe_batch(self, body: RtspBatchBody) -> ApiEnvelope:
        return ApiEnvelope.ok(data=[self.probe_item(rtsp) for rtsp in body.rtsps])

    def probe_response(self, rtsp: str) -> ApiEnvelope:
        item = self.probe_item(rtsp)
        envelope = ApiEnvelope.fail(item.get("error") or "probe failed")
        if item.get("success"):
            envelope = ApiEnvelope.ok(
                data={"resolution": item["resolution"], "fps": item["fps"]}
            )
        return envelope

    def probe_item(self, rtsp: str) -> dict:
        item = {"rtsp": rtsp, "success": False, "error": "rtsp must start with rtsp://"}
        if rtsp.startswith("rtsp://"):
            completed = self.runner.run_soft([*FFPROBE_BASE, rtsp])
            if completed.returncode != 0:
                item["error"] = self.fail_detail(completed)
            else:
                probe_text = completed.stdout.strip()
                probe = json.loads(probe_text) if probe_text else {}
                summary = self.mapper.summarize(probe)
                if summary is None:
                    item["error"] = "no video stream in probe"
                else:
                    item = {
                        "rtsp": rtsp,
                        "success": True,
                        "resolution": summary["resolution"],
                        "fps": summary["fps"],
                    }
        return item

    def fail_detail(self, completed) -> str:
        stderr = self.usable_probe_text(completed.stderr or "")
        stdout = self.usable_probe_text(completed.stdout or "")
        detail = stderr or stdout or "ffprobe failed"
        return detail

    def usable_probe_text(self, text: str) -> str:
        cleaned = text.strip()
        detail = ""
        if cleaned:
            compact = "".join(cleaned.split())
            if compact not in {"{}", "[]", "null"}:
                lines = [line.strip() for line in cleaned.splitlines() if line.strip()]
                detail = lines[-1] if lines else cleaned
        return detail


def default_mediamtx_host() -> str:
    return f"{PROJECT_NAME}_mediamtx"
