"""End-to-end tests: real ffmpeg/ffprobe produce a clip under locked/ from rolling/.

Requires ffmpeg + ffprobe on PATH or under /usr/local/bin/ (container layout).
"""

import os
import shutil
import subprocess
import sys
import time
from datetime import datetime, timedelta, timezone
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import recording.clip_extractor as clip_extractor_mod
from recording.clip_extractor import RollingClipExtractor
from utils.storage import StorageManager


def _resolve_ffmpeg_ffprobe():
    ff = shutil.which("ffmpeg")
    fp = shutil.which("ffprobe")
    if ff and fp:
        return ff, fp
    cff = Path("/usr/local/bin/ffmpeg")
    cfp = Path("/usr/local/bin/ffprobe")
    if cff.is_file() and cfp.is_file():
        return str(cff), str(cfp)
    return None, None


@pytest.fixture(scope="module")
def ffmpeg_ffprobe():
    pair = _resolve_ffmpeg_ffprobe()
    if not pair[0] or not pair[1]:
        pytest.skip("ffmpeg and ffprobe required (PATH or /usr/local/bin)")
    return pair[0], pair[1]


def _write_lavfi_mp4(ffmpeg_bin: str, dest: Path, duration_sec: int = 4) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        ffmpeg_bin,
        "-y",
        "-f", "lavfi",
        "-i", f"testsrc=duration={duration_sec}:size=160x120:rate=1",
        "-c:v", "libx264",
        "-pix_fmt", "yuv420p",
        "-preset", "ultrafast",
        str(dest),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    if proc.returncode != 0:
        cmd_mpeg = [
            ffmpeg_bin,
            "-y",
            "-f", "lavfi",
            "-i", f"testsrc=duration={duration_sec}:size=160x120:rate=1",
            "-c:v", "mpeg4",
            str(dest),
        ]
        proc2 = subprocess.run(cmd_mpeg, capture_output=True, text=True, timeout=120)
        if proc2.returncode != 0:
            raise RuntimeError(
                f"failed to generate test mp4: {proc.stderr}\n{proc2.stderr}",
            )


def _ffprobe_duration(ffprobe_bin: str, path: Path) -> float:
    cmd = [
        ffprobe_bin,
        "-v", "error",
        "-show_entries", "format=duration",
        "-of", "default=noprint_wrappers=1:nokey=1",
        str(path),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr)
    return float(proc.stdout.strip())


class TestRollingClipExtractorE2E:

    def test_single_segment_writes_locked_mp4(self, tmp_path, ffmpeg_ffprobe, monkeypatch):
        ffmpeg_bin, ffprobe_bin = ffmpeg_ffprobe
        monkeypatch.setattr(clip_extractor_mod, "_FFMPEG", ffmpeg_bin)
        monkeypatch.setattr(clip_extractor_mod, "_FFPROBE", ffprobe_bin)

        base = tmp_path / "storage"
        cam = "cam_e2e_single"
        roll = base / cam / "rolling"
        _write_lavfi_mp4(ffmpeg_bin, roll / "seg0.mp4", duration_sec=5)

        sm = StorageManager(base_dir=str(base))
        sm.ensure_dirs(cam)

        mtime = datetime.fromtimestamp((roll / "seg0.mp4").stat().st_mtime, tz=timezone.utc)
        dur = _ffprobe_duration(ffprobe_bin, roll / "seg0.mp4")
        wall_start_seg = mtime - timedelta(seconds=dur)
        window_start = wall_start_seg + timedelta(seconds=0.5)
        window_end = mtime - timedelta(seconds=0.5)

        ex = RollingClipExtractor(sm)
        out = ex.extract(cam, window_start, window_end, "e2e-single-req")

        assert out.is_file()
        assert out.parent == sm.locked_dir(cam)
        assert out.name == "clip_e2e-single-req.mp4"
        assert out.stat().st_size > 500
        locked_dur = _ffprobe_duration(ffprobe_bin, out)
        assert locked_dur > 0.5

    def test_two_segments_concat_writes_locked_mp4(self, tmp_path, ffmpeg_ffprobe, monkeypatch):
        ffmpeg_bin, ffprobe_bin = ffmpeg_ffprobe
        monkeypatch.setattr(clip_extractor_mod, "_FFMPEG", ffmpeg_bin)
        monkeypatch.setattr(clip_extractor_mod, "_FFPROBE", ffprobe_bin)

        base = tmp_path / "storage"
        cam = "cam_e2e_multi"
        roll = base / cam / "rolling"
        roll.mkdir(parents=True, exist_ok=True)

        f1 = roll / "a.mp4"
        f2 = roll / "b.mp4"
        _write_lavfi_mp4(ffmpeg_bin, f1, duration_sec=3)
        time.sleep(0.05)
        _write_lavfi_mp4(ffmpeg_bin, f2, duration_sec=3)

        t1 = 1_700_000_000.0
        t2 = t1 + 4.0
        os.utime(f1, (t1, t1))
        os.utime(f2, (t2, t2))

        d1 = _ffprobe_duration(ffprobe_bin, f1)
        d2 = _ffprobe_duration(ffprobe_bin, f2)
        end1 = datetime.fromtimestamp(t1, tz=timezone.utc)
        start1 = end1 - timedelta(seconds=d1)
        end2 = datetime.fromtimestamp(t2, tz=timezone.utc)
        start2 = end2 - timedelta(seconds=d2)

        window_start = start1 + timedelta(seconds=0.2)
        window_end = end2 - timedelta(seconds=0.2)
        assert window_start < window_end
        assert start1 < window_end and end2 > window_start

        sm = StorageManager(base_dir=str(base))
        sm.ensure_dirs(cam)

        ex = RollingClipExtractor(sm)
        out = ex.extract(cam, window_start, window_end, "e2e-multi-req")

        assert out.is_file()
        assert out.stat().st_size > 500
        locked_dur = _ffprobe_duration(ffprobe_bin, out)
        assert locked_dur > 1.0
