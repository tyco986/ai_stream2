#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent

SCRIPTS = (
    "test_hello_world.py",
    "test_remove_B_frame.py",
    "test_frame_extract.py",
    "test_video2rtsp.py",
    "test_video2rtsp_list.py",
    "test_rtsp_info.py",
    "test_video2rtsp_stop.py",
)


def run_script(name: str, extra_args: list[str]) -> subprocess.CompletedProcess[str]:
    cmd = [sys.executable, str(TESTS_DIR / name), *extra_args]
    print(f"\n=== {name} ===")
    return subprocess.run(cmd, text=True, capture_output=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    common = ["--host", args.host, "--port", str(args.port)]

    for name in SCRIPTS:
        result = run_script(name, common)
        if result.stdout:
            print(result.stdout, end="")
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        if result.returncode != 0:
            return result.returncode

    print("\nALL OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
