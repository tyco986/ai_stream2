#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 [--input PATH]

  --input PATH  ffmpeg/video/... or ffmpeg/output/... (default: ffmpeg/video/video1.mp4)
                Mapped to /app/video/ or /app/output/ inside container.

Run from project root. rtsp and loop use API defaults.
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

case "$INPUT" in
  ffmpeg/output/*) API_INPUT="/app/output/${INPUT#ffmpeg/output/}" ;;
  *) API_INPUT="/app/video/${INPUT#ffmpeg/video/}" ;;
esac

curl -sfS -X POST http://127.0.0.1:8080/ffmpeg/video2rtsp \
  -H 'Content-Type: application/json' \
  -d "{\"input\":\"${API_INPUT}\"}"
echo
