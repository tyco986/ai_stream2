#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_api_curl.sh
. "${SCRIPT_DIR}/_api_curl.sh"

usage() {
  cat <<EOF
Usage: $0 [--input PATH]

  --input PATH  attachments/videos/... (default: attachments/videos/video1.mp4)
                Mapped to /app/video/... inside ai_stream2_ffmpeg.

Run from project root. Requires container on http://127.0.0.1:8080.
EOF
}

INPUT="attachments/videos/video1.mp4"
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --input)
      [[ $# -ge 2 ]] || { usage >&2; exit 1; }
      INPUT="$2"
      shift 2
      ;;
    *) usage >&2; exit 1 ;;
  esac
done

case "$INPUT" in
  attachments/videos/*) API_INPUT="/app/video/${INPUT#attachments/videos/}" ;;
  /app/*) API_INPUT="$INPUT" ;;
  *) API_INPUT="/app/video/${INPUT}" ;;
esac

api_curl -X POST http://127.0.0.1:8080/ffmpeg/remove_B_frame \
  -H 'Content-Type: application/json' \
  -d "{\"input\":\"${API_INPUT}\"}"
echo
