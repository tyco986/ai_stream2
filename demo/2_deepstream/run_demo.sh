#!/usr/bin/env bash
# Run YOLO26 DeepStream demo from the host (docker exec into ai_stream2_deepstream).
#
# Prerequisites:
#   ./demo/1_mediamtx/1_build_container.sh   # MediaMTX on ai_stream2_default
#   ./demo/2_deepstream/2_build_container.sh   # DeepStream container running
#
# Host playback (after pipeline starts; path = {input_path_stem}_ai):
#   RTSP:   rtsp://127.0.0.1:8554/video1_B0_ai
#   WebRTC: http://127.0.0.1:8889/video1_B0_ai
#
# Usage: ./run_demo.sh
#   RTSP_URLS='rtsp://ai_stream2_mediamtx:8554/video1' ./run_demo.sh  # -> video1_ai
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CONTAINER="${DEEPSTREAM_CONTAINER_NAME:-ai_stream2_deepstream}"
MEDIAMTX="${MEDIAMTX_CONTAINER_NAME:-ai_stream2_mediamtx}"
NET="${DEEPSTREAM_NETWORK:-ai_stream2_default}"

# Pull URLs (comma-separated). Output path = {last URL path segment}_ai (main.py paths_for_sources).
RTSP_URLS="${RTSP_URLS:-rtsp://${MEDIAMTX}:8554/video1_B0}"
PGIE_CONFIG="${PGIE_CONFIG:-/app/config/pgie_yolo26_person.yml}"

command -v docker >/dev/null || { echo "ERROR: docker required" >&2; exit 1; }

docker ps --format '{{.Names}}' | grep -qx "${CONTAINER}" || {
  echo "ERROR: container ${CONTAINER} is not running. Run: ${SCRIPT_DIR}/2_build_container.sh" >&2
  exit 1
}

docker ps --format '{{.Names}}' | grep -qx "${MEDIAMTX}" || {
  echo "ERROR: container ${MEDIAMTX} is not running. Run: ${ROOT}/demo/1_mediamtx/1_build_container.sh" >&2
  exit 1
}

_first_rtsp="${RTSP_URLS%%,*}"
_first_rtsp="${_first_rtsp%%\?*}"
_in_stem="${_first_rtsp##*/}"
PLAY_PATH="${_in_stem}_ai"
echo "=> DeepStream demo (${NET})"
echo "   in:  ${RTSP_URLS}"
echo "   pgie: ${PGIE_CONFIG}"
echo
echo "   宿主机播放（path = {输入 path}_ai）："
echo "     rtsp://127.0.0.1:8554/${PLAY_PATH}"
echo "     http://127.0.0.1:8889/${PLAY_PATH}"
echo "   Ctrl+C to stop."
echo

exec docker exec -it \
  -e DEEPSTREAM_MODE=static \
  -e VISUALIZATION_RTSP=1 \
  -e RTSP_URLS="${RTSP_URLS}" \
  -e MEDIAMTX_HOST="${MEDIAMTX}" \
  -e MEDIAMTX_RTSP_PORT="${MEDIAMTX_RTSP_PORT:-8554}" \
  -e PGIE_CONFIG="${PGIE_CONFIG}" \
  -w /app \
  "${CONTAINER}" \
  python3 main.py
