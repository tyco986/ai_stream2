#!/usr/bin/env python3
import argparse
import sys

import requests

from _common import DEFAULT_API_VIDEO, print_ok


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--input", default=DEFAULT_API_VIDEO)
    parser.add_argument("--loop", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--rtsp", default=None)
    args = parser.parse_args()

    payload = {"input": args.input, "loop": args.loop}
    if args.rtsp is not None:
        payload["rtsp"] = args.rtsp

    url = f"http://{args.host}:{args.port}/ffmpeg/video2rtsp"
    resp = requests.post(url, json=payload, timeout=30)
    if resp.status_code != 200:
        print(f"FAIL status={resp.status_code} body={resp.text}", file=sys.stderr)
        return 1

    data = resp.json()
    if not data.get("pid") or not data.get("rtsp"):
        print(f"FAIL missing pid/rtsp: {data}", file=sys.stderr)
        return 1

    print_ok(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
