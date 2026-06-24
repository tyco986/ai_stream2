#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

API_URL="http://127.0.0.1:8080"
ENDPOINT="${API_URL}/ai_stream2/ffmpeg/remove_B_frame"

usage() {
  cat <<EOF
usage: $0 --input path/to/video --output path/to/video.mp4

Remove B-frames via FFmpeg API.

Options:
  --input PATH         Video file (required)
  --output PATH        Output MP4 path (required)

Prerequisites: 1_build_image.sh, 2_run_container.sh
EOF
}

INPUT=""
OUTPUT=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="$2"
      shift 2
      ;;
    --output)
      OUTPUT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

[[ -n "$INPUT" ]] || { echo "--input is required" >&2; usage; exit 1; }
[[ -n "$OUTPUT" ]] || { echo "--output is required" >&2; usage; exit 1; }
[[ -f "$INPUT" ]] || { echo "input not found: $INPUT" >&2; exit 1; }

mkdir -p "$(dirname "$OUTPUT")"
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -F "input=@${INPUT}")"

if [[ "${HTTP_CODE}" == "200" ]]; then
  mv "${RESPONSE_BODY}" "${OUTPUT}"
  RESPONSE_BODY=""
else
  cat "${RESPONSE_BODY}"
  exit 1
fi
