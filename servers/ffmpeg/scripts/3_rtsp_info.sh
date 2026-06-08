#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_api_curl.sh
. "${SCRIPT_DIR}/_api_curl.sh"

usage() {
  cat <<EOF
Usage: $0 [--rtsp URL]

  --rtsp URL  RTSP URL to probe (default: rtsp://ai_stream2_mediamtx:8554/video1_B0)
EOF
}

RTSP="rtsp://ai_stream2_mediamtx:8554/video1"
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --rtsp)
      [[ $# -ge 2 ]] || { usage >&2; exit 1; }
      RTSP="$2"
      shift 2
      ;;
    *) usage >&2; exit 1 ;;
  esac
done

api_curl -X POST http://127.0.0.1:8080/ffmpeg/rtsp_info \
  -H 'Content-Type: application/json' \
  -d "{\"rtsp\":\"${RTSP}\"}"
echo
