#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 [--input PATH]

  --input PATH  Host path under ffmpeg/video/ (default: ffmpeg/video/video1.mp4)
                Mapped to /app/video/... inside ai_stream2_ffmpeg.

Run from project root. Requires container on http://127.0.0.1:8080.
EOF
}

INPUT="ffmpeg/video/video1.mp4"
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

API_INPUT="/app/video/${INPUT#ffmpeg/video/}"

curl -sfS -X POST http://127.0.0.1:8080/ffmpeg/remove_B_frame \
  -H 'Content-Type: application/json' \
  -d "{\"input\":\"${API_INPUT}\"}"
echo
