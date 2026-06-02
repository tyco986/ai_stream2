#!/usr/bin/env bash
set -euo pipefail

RTSP="rtsp://127.0.0.1:8554/video1"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --rtsp) RTSP="$2"; shift 2 ;;
    *) echo "usage: $0 [--rtsp URL]" >&2; exit 1 ;;
  esac
done
PATH_NAME="${RTSP##*/}"; PATH_NAME="${PATH_NAME%%\?*}"

curl -s -X POST "http://127.0.0.1:9997/v3/config/paths/replace/${PATH_NAME}" \
  -H "Content-Type: application/json" \
  -d '{"record":false}'
