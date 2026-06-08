#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_api_curl.sh
. "${SCRIPT_DIR}/_api_curl.sh"

usage() {
  cat <<EOF
Usage: $0 [--help]

  POST http://127.0.0.1:8080/ffmpeg/video2rtsp_stop
  Body: {"rtsp":"all"}  (stops every active publisher)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

api_curl -X POST http://127.0.0.1:8080/ffmpeg/video2rtsp_stop \
  -H 'Content-Type: application/json' \
  -d '{"rtsp":"all"}'
echo
