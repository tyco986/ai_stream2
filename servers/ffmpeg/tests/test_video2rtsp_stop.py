#!/usr/bin/env python3
import argparse
import sys

import requests

from _common import print_ok


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--rtsp", default="all")
    args = parser.parse_args()

    url = f"http://{args.host}:{args.port}/ffmpeg/video2rtsp_stop"
    resp = requests.post(url, json={"rtsp": args.rtsp}, timeout=30)
    if resp.status_code != 200:
        print(f"FAIL status={resp.status_code} body={resp.text}", file=sys.stderr)
        return 1

    data = resp.json()
    stopped = data.get("stopped")
    if not isinstance(stopped, list):
        print(f"FAIL missing stopped list: {data}", file=sys.stderr)
        return 1

    print_ok(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
