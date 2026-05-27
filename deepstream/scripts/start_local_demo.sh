#!/usr/bin/env bash
# One-shot local demo: Kafka + DeepStream (YOLOv10), publish test RTSP streams from inside deepstream container, REST stream/add.
# Run from anywhere:  bash deepstream/script/start_local_demo.sh
# Requires: docker compose, curl; NVIDIA GPU for DeepStream.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${PROJECT_ROOT}"

export DS_LIGHT_PIPELINE="${DS_LIGHT_PIPELINE:-0}"
# Keep rolling segments short for Kafka E2E tests that start rolling explicitly.
export DS_RECORDING_SEGMENT_SEC="${DS_RECORDING_SEGMENT_SEC:-30}"
export REBUILD="${REBUILD:-0}"

DS_REST="${DS_REST:-http://127.0.0.1:9000}"
MEDIAMTX_RTSP_BASE="${MEDIAMTX_RTSP_BASE:-rtsp://127.0.0.1:8554}"
READY_TIMEOUT_SEC="${READY_TIMEOUT_SEC:-600}"
READY_POLL_SEC="${READY_POLL_SEC:-5}"

VIDEO1="${VIDEO1:-${PROJECT_ROOT}/deepstream/example_data/video1_bf0.mp4}"
VIDEO2="${VIDEO2:-${PROJECT_ROOT}/deepstream/example_data/video2_bf0.mp4}"
STREAM1="${STREAM1:-cam1}"
STREAM2="${STREAM2:-cam2}"
CAMERA_ID1="${CAMERA_ID1:-demo_cam1}"
CAMERA_ID2="${CAMERA_ID2:-demo_cam2}"
CAMERA_NAME1="${CAMERA_NAME1:-Demo Cam 1}"
CAMERA_NAME2="${CAMERA_NAME2:-Demo Cam 2}"

LOG_FILE="${PROJECT_ROOT}/deepstream/storage/video2rtsp.log"
PID_FILE="${PROJECT_ROOT}/deepstream/storage/.video2rtsp.pid"
DEEPSTREAM_SERVICE="${DEEPSTREAM_SERVICE:-deepstream}"

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing command: $1" >&2
    exit 1
  }
}

require_cmd docker
require_cmd curl

ds_ready() {
  curl -sS --connect-timeout 3 "${DS_REST}/api/v1/health/get-dsready-state" \
    | grep -q '"ds-ready"[[:space:]]*:[[:space:]]*"YES"'
}

remove_stream() {
  local camera_id="$1"
  local camera_url="$2"
  curl -sS -X POST "${DS_REST}/api/v1/stream/remove" \
    -H 'Content-Type: application/json' \
    -d "{\"key\":\"sensor\",\"value\":{\"camera_id\":\"${camera_id}\",\"camera_url\":\"${camera_url}\",\"change\":\"camera_remove\"}}" \
    >/dev/null || true
}

add_stream() {
  local camera_id="$1"
  local camera_name="$2"
  local camera_url="$3"
  curl -sS -X POST "${DS_REST}/api/v1/stream/add" \
    -H 'Content-Type: application/json' \
    -d "{\"key\":\"sensor\",\"value\":{\"camera_id\":\"${camera_id}\",\"camera_name\":\"${camera_name}\",\"camera_url\":\"${camera_url}\",\"change\":\"camera_add\"}}" \
    | head -c 200
  echo
}

mkdir -p "${PROJECT_ROOT}/deepstream/storage"

if [[ "${REBUILD}" == "1" ]]; then
  echo "=> docker compose build deepstream ..."
  docker compose build deepstream
fi

echo "=> docker compose up -d kafka deepstream (DS_LIGHT_PIPELINE=${DS_LIGHT_PIPELINE}, DS_RECORDING_SEGMENT_SEC=${DS_RECORDING_SEGMENT_SEC}) ..."
DS_LIGHT_PIPELINE="${DS_LIGHT_PIPELINE}" DS_RECORDING_SEGMENT_SEC="${DS_RECORDING_SEGMENT_SEC}" docker compose up -d kafka deepstream

echo "=> Waiting for ds-ready (timeout ${READY_TIMEOUT_SEC}s, TensorRT may build on first run) ..."
elapsed=0
ready=0
while [[ "${elapsed}" -lt "${READY_TIMEOUT_SEC}" ]]; do
  if ds_ready; then
    ready=1
    echo "ds-ready: YES"
    break
  fi
  sleep "${READY_POLL_SEC}"
  elapsed=$((elapsed + READY_POLL_SEC))
  echo "  ... ${elapsed}s (still waiting)"
done

if [[ "${ready}" != "1" ]]; then
  echo "Timed out waiting for ds-ready. Check: docker logs \$(docker compose ps -q deepstream)" >&2
  exit 1
fi

if [[ -x "${SCRIPT_DIR}/stop_local_demo.sh" ]]; then
  "${SCRIPT_DIR}/stop_local_demo.sh" >/dev/null 2>&1 || true
fi

VIDEO1_IN_CONTAINER="/app/example_data/$(basename "${VIDEO1}")"
VIDEO2_IN_CONTAINER="/app/example_data/$(basename "${VIDEO2}")"

echo "=> Starting video2rtsp inside container ${DEEPSTREAM_SERVICE} (log: ${LOG_FILE}) ..."
container_pid="$(
  docker compose exec -T "${DEEPSTREAM_SERVICE}" sh -lc "nohup python3 /app/script/video2rtsp.py \
  --input \"${VIDEO1_IN_CONTAINER}:${STREAM1}\" \"${VIDEO2_IN_CONTAINER}:${STREAM2}\" \
  --loop \
  --mediamtx \"${MEDIAMTX_RTSP_BASE}\" \
  --mode webrtc \
  >/app/storage/video2rtsp.log 2>&1 & echo \$!"
)"

if ! docker compose exec -T "${DEEPSTREAM_SERVICE}" sh -lc "kill -0 ${container_pid}" >/dev/null 2>&1; then
  echo "Failed to start video2rtsp inside container. Check /app/storage/video2rtsp.log in deepstream container." >&2
  exit 1
fi

echo "container:${container_pid}" >"${PID_FILE}"

sleep "${PUBLISH_WAIT_SEC:-5}"

RTSP1="${MEDIAMTX_RTSP_BASE%/}/${STREAM1}"
RTSP2="${MEDIAMTX_RTSP_BASE%/}/${STREAM2}"

echo "=> stream/remove (ignore errors) + stream/add ..."

remove_stream "${CAMERA_ID1}" "${RTSP1}"
remove_stream "${CAMERA_ID2}" "${RTSP2}"
add_stream "${CAMERA_ID1}" "${CAMERA_NAME1}" "${RTSP1}"
add_stream "${CAMERA_ID2}" "${CAMERA_NAME2}" "${RTSP2}"

echo
echo "Done."
echo "  WebRTC preview:  http://127.0.0.1:8889/preview/"
echo "  DeepStream REST: ${DS_REST}"
echo "  video2rtsp PID:  $(cat "${PID_FILE}")  (stop: ${SCRIPT_DIR}/stop_local_demo.sh)"
echo "  ffmpeg log:      ${LOG_FILE}"
