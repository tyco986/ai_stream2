#!/usr/bin/env bash
# One-shot local demo: Kafka + DeepStream (YOLOv10), ffmpeg RTSP publish, REST stream/add.
# Run from anywhere:  bash deepstream/script/start_local_demo.sh
# Requires: docker compose, python3, ffmpeg, ffprobe; NVIDIA GPU for DeepStream.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${PROJECT_ROOT}"

export DS_LIGHT_PIPELINE="${DS_LIGHT_PIPELINE:-0}"
# Test/demo: short rolling segments (override for production-style long segments)
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

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing command: $1" >&2
    exit 1
  }
}

require_cmd docker
require_cmd python3
require_cmd ffmpeg
require_cmd ffprobe
require_cmd curl

if [[ ! -f "${VIDEO1}" || ! -f "${VIDEO2}" ]]; then
  echo "Video files not found. Expected:" >&2
  echo "  ${VIDEO1}" >&2
  echo "  ${VIDEO2}" >&2
  exit 1
fi

if [[ "${REBUILD}" == "1" ]]; then
  echo "=> docker compose build deepstream ..."
  docker compose build deepstream
fi

echo "=> docker compose up -d kafka deepstream (DS_LIGHT_PIPELINE=${DS_LIGHT_PIPELINE}, DS_RECORDING_SEGMENT_SEC=${DS_RECORDING_SEGMENT_SEC}) ..."
DS_LIGHT_PIPELINE="${DS_LIGHT_PIPELINE}" DS_RECORDING_SEGMENT_SEC="${DS_RECORDING_SEGMENT_SEC}" docker compose up -d kafka deepstream

echo "=> Waiting for ds-ready (timeout ${READY_TIMEOUT_SEC}s, TensorRT may build on first run) ..."
elapsed=0
while [[ "${elapsed}" -lt "${READY_TIMEOUT_SEC}" ]]; do
  if curl -sS --connect-timeout 3 "${DS_REST}/api/v1/health/get-dsready-state" | grep -q '"ds-ready"[[:space:]]*:[[:space:]]*"YES"'; then
    echo "ds-ready: YES"
    break
  fi
  sleep "${READY_POLL_SEC}"
  elapsed=$((elapsed + READY_POLL_SEC))
  echo "  ... ${elapsed}s (still waiting)"
done

if ! curl -sS --connect-timeout 3 "${DS_REST}/api/v1/health/get-dsready-state" | grep -q '"ds-ready"[[:space:]]*:[[:space:]]*"YES"'; then
  echo "Timed out waiting for ds-ready. Check: docker logs \$(docker compose ps -q deepstream)" >&2
  exit 1
fi

if [[ -f "${PID_FILE}" ]] && kill -0 "$(cat "${PID_FILE}")" 2>/dev/null; then
  echo "Stopping previous video2rtsp (PID $(cat "${PID_FILE}")) ..."
  kill -TERM "$(cat "${PID_FILE}")" 2>/dev/null || true
  sleep 1
fi

mkdir -p "${PROJECT_ROOT}/deepstream/storage"
echo "=> Starting video2rtsp (log: ${LOG_FILE}) ..."
nohup python3 "${PROJECT_ROOT}/deepstream/script/video2rtsp.py" \
  --input "${VIDEO1}:${STREAM1}" "${VIDEO2}:${STREAM2}" \
  --loop \
  --mediamtx "${MEDIAMTX_RTSP_BASE}" \
  --mode webrtc \
  >>"${LOG_FILE}" 2>&1 &
echo $! >"${PID_FILE}"

sleep "${PUBLISH_WAIT_SEC:-5}"

RTSP1="${MEDIAMTX_RTSP_BASE%/}/${STREAM1}"
RTSP2="${MEDIAMTX_RTSP_BASE%/}/${STREAM2}"

echo "=> stream/remove (ignore errors) + stream/add ..."

curl -sS -X POST "${DS_REST}/api/v1/stream/remove" \
  -H 'Content-Type: application/json' \
  -d "{\"key\":\"sensor\",\"value\":{\"camera_id\":\"${CAMERA_ID1}\",\"camera_url\":\"${RTSP1}\",\"change\":\"camera_remove\"}}" \
  >/dev/null || true
curl -sS -X POST "${DS_REST}/api/v1/stream/remove" \
  -H 'Content-Type: application/json' \
  -d "{\"key\":\"sensor\",\"value\":{\"camera_id\":\"${CAMERA_ID2}\",\"camera_url\":\"${RTSP2}\",\"change\":\"camera_remove\"}}" \
  >/dev/null || true

curl -sS -X POST "${DS_REST}/api/v1/stream/add" \
  -H 'Content-Type: application/json' \
  -d "{\"key\":\"sensor\",\"value\":{\"camera_id\":\"${CAMERA_ID1}\",\"camera_name\":\"${CAMERA_NAME1}\",\"camera_url\":\"${RTSP1}\",\"change\":\"camera_add\"}}" | head -c 200
echo
curl -sS -X POST "${DS_REST}/api/v1/stream/add" \
  -H 'Content-Type: application/json' \
  -d "{\"key\":\"sensor\",\"value\":{\"camera_id\":\"${CAMERA_ID2}\",\"camera_name\":\"${CAMERA_NAME2}\",\"camera_url\":\"${RTSP2}\",\"change\":\"camera_add\"}}" | head -c 200
echo

echo
echo "Done."
echo "  WebRTC preview:  http://127.0.0.1:8889/preview/"
echo "  DeepStream REST: ${DS_REST}"
echo "  video2rtsp PID:  $(cat "${PID_FILE}")  (stop: ${SCRIPT_DIR}/stop_local_demo.sh)"
echo "  ffmpeg log:      ${LOG_FILE}"
