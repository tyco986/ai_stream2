#!/usr/bin/env bash
# Run from project root (or invoke directly).
# Start platform + Servers-page infrastructure containers, then publish video1/video2.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=./load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
cd "${ROOT}"

VIDEO1="${ROOT}/attachments/videos/video1.mp4"
VIDEO2="${ROOT}/attachments/videos/video2.mp4"
FFMPEG_HEALTH="http://127.0.0.1:8080/${PROJECT_NAME}/ffmpeg/health"
MEDIAMTX_HEALTH="http://127.0.0.1:9997/v3/info"
PUBLISHERS="${ROOT}/servers/ffmpeg/scripts/3_publishers.sh"

run_script() {
  local path="$1"
  echo "==> ${path}"
  bash "${path}"
}

wait_http() {
  local url="$1"
  local label="$2"
  local attempts="${3:-60}"
  local i
  for ((i = 1; i <= attempts; i++)); do
    if curl -sf "${url}" >/dev/null; then
      echo "${label} ready"
      return 0
    fi
    sleep 1
  done
  echo "${label} not ready: ${url}" >&2
  exit 1
}

[[ -f "${VIDEO1}" ]] || { echo "missing ${VIDEO1}" >&2; exit 1; }
[[ -f "${VIDEO2}" ]] || { echo "missing ${VIDEO2}" >&2; exit 1; }

# Platform (django / nodejs / pipeline lifecycle)
run_script "${ROOT}/servers/postgresql/scripts/1_run_container.sh"
run_script "${ROOT}/servers/docker_socket_proxy/scripts/1_run_container.sh"

# Servers registry: infrastructure
run_script "${ROOT}/servers/mediamtx/scripts/1_run_container.sh"
run_script "${ROOT}/servers/kafka/scripts/1_run_container.sh"
run_script "${ROOT}/servers/ffmpeg/scripts/2_run_dev_container.sh"
run_script "${ROOT}/servers/generator/scripts/2_run_dev_container.sh"
run_script "${ROOT}/servers/export_onnx/scripts/2_run_container.sh"
run_script "${ROOT}/servers/export_trt/scripts/2_run_dev_container.sh"
run_script "${ROOT}/servers/django/scripts/2_run_dev_container.sh"
run_script "${ROOT}/servers/nodejs/scripts/2_run_dev_container.sh"

wait_http "${MEDIAMTX_HEALTH}" "mediamtx"
wait_http "${FFMPEG_HEALTH}" "ffmpeg"

echo "==> publish video1 / video2"
bash "${PUBLISHERS}" --input "${VIDEO1}" --name video1
bash "${PUBLISHERS}" --input "${VIDEO2}" --name video2

echo "demo ready"
echo "  UI:     http://127.0.0.1:5173"
echo "  RTSP:   rtsp://127.0.0.1:8554/video1"
echo "  RTSP:   rtsp://127.0.0.1:8554/video2"
echo "  RTSP:   rtsp://${PROJECT_NAME}_mediamtx:8554/video1"
echo "  RTSP:   rtsp://${PROJECT_NAME}_mediamtx:8554/video2"
