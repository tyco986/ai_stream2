#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

RTSP="rtsp://127.0.0.1:8554/video1"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rtsp)
      RTSP="${2:?missing rtsp url}"
      shift 2
      ;;
    -h|--help)
      echo "Usage: $0 [--rtsp URL]"
      echo "Default: rtsp://127.0.0.1:8554/video1"
      exit 0
      ;;
    *)
      RTSP="$1"
      shift
      ;;
  esac
done

exec ffplay -hide_banner -loglevel warning -rtsp_transport tcp -i "$RTSP"
