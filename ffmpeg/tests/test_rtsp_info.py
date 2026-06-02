#!/usr/bin/env python3
import argparse
import sys

import requests

from _common import print_ok

DEFAULT_RTSP = "rtsp://ai_stream2_mediamtx:8554/video1"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--rtsp", default=DEFAULT_RTSP)
    args = parser.parse_args()

    url = f"http://{args.host}:{args.port}/ffmpeg/rtsp_info"
    resp = requests.post(url, json={"rtsp": args.rtsp}, timeout=30)
    if resp.status_code != 200:
        print(f"FAIL status={resp.status_code} body={resp.text}", file=sys.stderr)
        return 1

    data = resp.json()
    probe = data.get("probe")
    if (
        data.get("rtsp") != args.rtsp
        or not isinstance(probe, dict)
        or not data.get("command")
    ):
        print(f"FAIL unexpected body: {data}", file=sys.stderr)
        return 1
    if "streams" not in probe and "format" not in probe:
        print(f"FAIL missing probe streams/format: {data}", file=sys.stderr)
        return 1

    print_ok(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
