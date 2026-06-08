#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
# shellcheck source=_api_curl.sh
. "${SCRIPT_DIR}/_api_curl.sh"

CONTAINER="ai_stream2_ffmpeg"
CONTAINER_VIDEO_DIR="/app/videos"
DEFAULT_INPUT="attachments/videos/video1.mp4"

usage() {
  cat <<EOF
Usage: $0 [--input PATH]

  --input PATH  Host video path (default: attachments/videos/video1.mp4)
                File is copied to ${CONTAINER_VIDEO_DIR}/ in ${CONTAINER}.

Run from project root. rtsp and loop use API defaults.
EOF
}

INPUT="$DEFAULT_INPUT"
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

if [[ "$INPUT" = /* ]]; then
  HOST_INPUT="$INPUT"
else
  HOST_INPUT="${ROOT}/${INPUT}"
fi

if [[ ! -f "$HOST_INPUT" ]]; then
  echo "Input not found: $HOST_INPUT" >&2
  exit 1
fi

if ! docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null | grep -q true; then
  echo "Container not running: $CONTAINER" >&2
  exit 1
fi

VIDEO_NAME="$(basename "$HOST_INPUT")"
CONTAINER_INPUT="${CONTAINER_VIDEO_DIR}/${VIDEO_NAME}"

docker exec "$CONTAINER" mkdir -p "$CONTAINER_VIDEO_DIR"
docker cp "$HOST_INPUT" "${CONTAINER}:${CONTAINER_INPUT}"

api_curl -X POST http://127.0.0.1:8080/ffmpeg/video2rtsp \
  -H 'Content-Type: application/json' \
  -d "{\"input\":\"${CONTAINER_INPUT}\"}"
echo
