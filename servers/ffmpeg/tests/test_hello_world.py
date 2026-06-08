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

    url = f"http://{args.host}:{args.port}/ffmpeg/hello_world"
    resp = requests.get(url, timeout=10)
    if resp.status_code != 200:
        print(f"FAIL status={resp.status_code} body={resp.text}", file=sys.stderr)
        return 1

    data = resp.json()
    if data != {"success": True, "message": "", "service": "ffmpeg"}:
        print(f"FAIL unexpected body={data}", file=sys.stderr)
        return 1

    print_ok(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
