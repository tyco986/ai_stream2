"""Extract a time window from rolling-record MP4 segments into locked/ using ffmpeg."""

import logging
import re
import shutil
import subprocess
import tempfile
import uuid
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path

from utils.storage import StorageManager

logger = logging.getLogger(__name__)

_FFMPEG = "/usr/local/bin/ffmpeg"
_FFPROBE = "/usr/local/bin/ffprobe"


class ClipExtractionError(Exception):
    """Raised when rolling segments cannot cover the window or ffmpeg fails."""


@dataclass(frozen=True)
class _Segment:
    path: Path
    wall_start: datetime
    wall_end: datetime


def parse_utc_iso(ts: str) -> datetime:
    """Parse ISO8601 timestamp to timezone-aware UTC."""
    s = ts.strip()
    if s.endswith("Z"):
        s = s[:-1] + "+00:00"
    dt = datetime.fromisoformat(s)
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    return dt.astimezone(timezone.utc)


def _safe_filename_fragment(request_id: str) -> str:
    cleaned = re.sub(r"[^a-zA-Z0-9._-]+", "_", request_id)
    return cleaned[:200] if cleaned else str(uuid.uuid4())


class RollingClipExtractor:
    """Build wall-clock coverage from rolling (and legacy) MP4s, then ffmpeg to locked/."""

    def __init__(self, storage: StorageManager):
        self._storage = storage

    def extract(
        self,
        camera_id: str,
        window_start: datetime,
        window_end: datetime,
        request_id: str,
    ) -> Path:
        if window_end <= window_start:
            raise ClipExtractionError("window_end must be after window_start")

        ws = window_start.astimezone(timezone.utc)
        we = window_end.astimezone(timezone.utc)

        segments = self._collect_segments(camera_id)
        overlapping = [s for s in segments if s.wall_start < we and s.wall_end > ws]
        overlapping.sort(key=lambda s: s.wall_start)

        if not overlapping:
            raise ClipExtractionError("no rolling segment overlaps the requested time window")

        out_dir = self._storage.locked_dir(camera_id)
        out_dir.mkdir(parents=True, exist_ok=True)
        out_name = f"clip_{_safe_filename_fragment(request_id)}.mp4"
        out_path = out_dir / out_name

        tmp_root = tempfile.mkdtemp(prefix="clip_")
        try:
            if len(overlapping) == 1:
                seg = overlapping[0]
                part = Path(tmp_root) / "part0.mp4"
                shutil.copy2(seg.path, part)
                eff_start = max(ws, seg.wall_start)
                eff_end = min(we, seg.wall_end)
                ss = (eff_start - seg.wall_start).total_seconds()
                dur = (eff_end - eff_start).total_seconds()
                if dur <= 0:
                    raise ClipExtractionError("effective window is empty after segment bounds")
                self._ffmpeg_trim(part, ss, dur, out_path)
            else:
                parts: list[Path] = []
                for i, seg in enumerate(overlapping):
                    eff_start = max(ws, seg.wall_start)
                    eff_end = min(we, seg.wall_end)
                    ss = (eff_start - seg.wall_start).total_seconds()
                    dur = (eff_end - eff_start).total_seconds()
                    if dur <= 0:
                        continue
                    part = Path(tmp_root) / f"part{i}.mp4"
                    shutil.copy2(seg.path, part)
                    trimmed = Path(tmp_root) / f"trim{i}.mp4"
                    self._ffmpeg_trim(part, ss, dur, trimmed)
                    parts.append(trimmed)
                if not parts:
                    raise ClipExtractionError("no usable segment overlap for extraction")
                if len(parts) == 1:
                    shutil.move(str(parts[0]), str(out_path))
                else:
                    self._ffmpeg_concat(parts, out_path)
        finally:
            shutil.rmtree(tmp_root, ignore_errors=True)

        logger.info(
            "Clip written: camera=%s request_id=%s path=%s",
            camera_id, request_id, out_path,
        )
        return out_path

    def _collect_segments(self, camera_id: str) -> list[_Segment]:
        paths: list[Path] = []
        roll = self._storage.rolling_dir(camera_id)
        leg = self._storage.legacy_recordings_dir(camera_id)
        if roll.is_dir():
            paths.extend(sorted(roll.glob("*.mp4")))
        if leg.is_dir():
            paths.extend(sorted(leg.glob("*.mp4")))

        segments: list[_Segment] = []
        for path in paths:
            try:
                dur = self._ffprobe_duration(path)
            except ClipExtractionError:
                logger.warning("Skipping unreadable segment: %s", path)
                continue
            if dur <= 0:
                continue
            mtime = datetime.fromtimestamp(path.stat().st_mtime, tz=timezone.utc)
            wall_start = mtime - timedelta(seconds=dur)
            wall_end = mtime
            segments.append(_Segment(path=path, wall_start=wall_start, wall_end=wall_end))
        return segments

    def _ffprobe_duration(self, path: Path) -> float:
        cmd = [
            _FFPROBE,
            "-v", "error",
            "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1",
            str(path),
        ]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            raise ClipExtractionError(f"ffprobe failed: {proc.stderr.strip()}")
        try:
            return float(proc.stdout.strip())
        except ValueError as e:
            raise ClipExtractionError(f"invalid ffprobe output: {proc.stdout!r}") from e

    def _ffmpeg_trim(self, src: Path, ss_sec: float, duration_sec: float, dst: Path) -> None:
        cmd = [
            _FFMPEG,
            "-y",
            "-ss", f"{ss_sec:.6f}",
            "-i", str(src),
            "-t", f"{duration_sec:.6f}",
            "-c", "copy",
            str(dst),
        ]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        if proc.returncode != 0:
            raise ClipExtractionError(f"ffmpeg trim failed: {proc.stderr[-2000:]}")

    def _ffmpeg_concat(self, parts: list[Path], dst: Path) -> None:
        list_path = dst.parent / f".concat_{uuid.uuid4().hex}.txt"
        try:
            lines = [f"file '{p.resolve()}'" for p in parts]
            list_path.write_text("\n".join(lines), encoding="utf-8")
            cmd = [
                _FFMPEG,
                "-y",
                "-f", "concat",
                "-safe", "0",
                "-i", str(list_path),
                "-c", "copy",
                str(dst),
            ]
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            if proc.returncode != 0:
                raise ClipExtractionError(f"ffmpeg concat failed: {proc.stderr[-2000:]}")
        finally:
            list_path.unlink(missing_ok=True)
