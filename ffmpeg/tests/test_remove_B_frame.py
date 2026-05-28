#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

import requests

from _common import print_ok

DEFAULT_INPUT = Path(__file__).resolve().parents[1] / "video" / "video1.mp4"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--input", default=str(DEFAULT_INPUT))
    parser.add_argument("--output", default=None)
    args = parser.parse_args()

    payload = {"input": args.input}
    if args.output is not None:
        payload["output"] = args.output

    url = f"http://{args.host}:{args.port}/ffmpeg/remove_B_frame"
    resp = requests.post(url, json=payload, timeout=600)
    if resp.status_code != 200:
        print(f"FAIL status={resp.status_code} body={resp.text}", file=sys.stderr)
        return 1

    data = resp.json()
    output = Path(data["output"])
    if not output.is_file() or output.stat().st_size == 0:
        print(f"FAIL output missing or empty: {output}", file=sys.stderr)
        return 1

    print_ok(data, file_path=output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
