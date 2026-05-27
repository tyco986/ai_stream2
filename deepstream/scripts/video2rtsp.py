"""Publish one or more video files as RTSP streams via FFmpeg + MediaMTX.

Usage:
    python video2rtsp.py \\
        --input video1.mp4:stream1 video2.mp4:stream2 \\
        --loop \\
        --mediamtx rtsp://127.0.0.1:8554 \\
        --mode webrtc|hls

Each ``video_path:stream_name`` pair spawns an FFmpeg subprocess that pushes
the video to ``rtsp://<mediamtx>/<stream_name>`` using copy-mode.

In webrtc mode, the script checks if the video contains B-frames. If B-frames
are present, publishing will fail with an error message.
"""
import argparse
import signal
import subprocess
import sys
from typing import List, Tuple


def has_b_frames(video_path: str) -> bool:
    """Detect if the video stream contains B-frames using ffprobe."""
    cmd = [
        "ffprobe",
        "-v", "error",
        "-select_streams", "v:0",
        "-show_entries", "stream=has_b_frames",
        "-of", "csv=p=0",
        video_path,
    ]
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=True,
        )
        value = result.stdout.strip()
        return value not in ("", "0")
    except (subprocess.CalledProcessError, FileNotFoundError):
        # If ffprobe fails or not found, assume B-frames may exist (fail safe)
        return True


def parse_inputs(raw: List[str]) -> List[Tuple[str, str]]:
    pairs: List[Tuple[str, str]] = []
    for item in raw:
        if ":" not in item:
            print(f"ERROR: invalid format '{item}', expected 'video_path:stream_name'")
            sys.exit(1)
        # Split on the LAST colon (in case path contains ':' on Windows)
        idx = item.rfind(":")
        video_path = item[:idx]
        stream_name = item[idx + 1:]
        pairs.append((video_path, stream_name))
    return pairs


def build_ffmpeg_cmd(
    video_path: str,
    rtsp_url: str,
    loop: bool,
) -> List[str]:
    cmd = ["ffmpeg"]
    if loop:
        cmd += ["-stream_loop", "-1"]
    cmd += [
        "-re",
        "-i", video_path,
        "-c", "copy",
        "-f", "rtsp",
        "-rtsp_transport", "tcp",
        rtsp_url,
    ]
    return cmd


class _ProcessShutdown:
    """Signal handler that terminates a list of subprocesses."""

    def __init__(self, processes: List[subprocess.Popen]):
        self._processes = processes

    def __call__(self, sig, frame):
        print("\nShutting down...")
        for p in self._processes:
            p.terminate()
        for p in self._processes:
            p.wait(timeout=5)
        sys.exit(0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Publish videos to RTSP via MediaMTX")
    parser.add_argument(
        "--input",
        nargs="+",
        required=True,
        help="video_path:stream_name pairs",
    )
    parser.add_argument(
        "--loop",
        action="store_true",
        help="Loop video playback",
    )
    parser.add_argument(
        "--mediamtx",
        default="rtsp://127.0.0.1:8554",
        help="MediaMTX RTSP base URL",
    )
    parser.add_argument(
        "--mode",
        choices=["webrtc", "hls"],
        default="webrtc",
        help="Stream mode: webrtc (rejects videos with B-frames) or hls",
    )
    args = parser.parse_args()

    pairs = parse_inputs(args.input)

    if args.mode == "webrtc":
        for video_path, stream_name in pairs:
            if has_b_frames(video_path):
                print(f"ERROR: Publishing failed. Video '{video_path}' contains B-frames, which are not supported in webrtc mode.")
                sys.exit(1)
    mediamtx_base = args.mediamtx.rstrip("/")

    processes: List[subprocess.Popen] = []

    shutdown = _ProcessShutdown(processes)
    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    for video_path, stream_name in pairs:
        rtsp_url = f"{mediamtx_base}/{stream_name}"
        cmd = build_ffmpeg_cmd(video_path, rtsp_url, args.loop)
        print(f"Starting: {' '.join(cmd)}")
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        processes.append(proc)

    print(f"Publishing {len(processes)} stream(s). Press Ctrl+C to stop.")

    # Wait for all processes
    for p in processes:
        p.wait()


if __name__ == "__main__":
    main()
