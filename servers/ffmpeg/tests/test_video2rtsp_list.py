#!/usr/bin/env python3
import argparse
import sys

import requests

from _common import print_ok


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    url = f"http://{args.host}:{args.port}/ffmpeg/video2rtsp_list"
    resp = requests.get(url, timeout=10)
    if resp.status_code != 200:
        print(f"FAIL status={resp.status_code} body={resp.text}", file=sys.stderr)
        return 1

    data = resp.json()
    publishers = data.get("publishers")
    if not isinstance(publishers, list):
        print(f"FAIL missing publishers list: {data}", file=sys.stderr)
        return 1

    for item in publishers:
        if not item.get("input") or not item.get("rtsp"):
            print(f"FAIL invalid publisher entry: {item}", file=sys.stderr)
            return 1

    print_ok(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
