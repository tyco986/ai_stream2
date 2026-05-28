#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

NAME="${FFMPEG_CONTAINER_NAME:-ai_stream2_ffmpeg}"
NET="${FFMPEG_NETWORK:-ai_stream2_default}"
IMG="${FFMPEG_IMAGE:-ai_stream2_ffmpeg}"
PORT="${FFMPEG_HOST_PORT:-8080}"
MEDIA_DIR="${FFMPEG_MEDIA_DIR:-${ROOT}/media}"
VIDEO_DIR="${FFMPEG_VIDEO_DIR:-${ROOT}/ffmpeg/video}"
OUTPUT_DIR="${FFMPEG_OUTPUT_DIR:-${ROOT}/ffmpeg/output}"
LOG_DIR="${FFMPEG_LOG_DIR:-${ROOT}/ffmpeg/log}"
MEDIAMTX_HOST="${MEDIAMTX_HOST:-ai_stream2_mediamtx}"
MEDIAMTX_RTSP_PORT="${MEDIAMTX_RTSP_PORT:-8554}"

mkdir -p "${MEDIA_DIR}" "${OUTPUT_DIR}" "${LOG_DIR}"

docker network create "${NET}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d --name "${NAME}" --network "${NET}" \
  -p "${PORT}:8080" \
  -v "${OUTPUT_DIR}:/app/output" \
  -v "${LOG_DIR}:/app/log" \
  -v "${MEDIA_DIR}:/media:ro" \
  -v "${VIDEO_DIR}:/app/video:ro" \
  -e MEDIAMTX_HOST="${MEDIAMTX_HOST}" \
  -e MEDIAMTX_RTSP_PORT="${MEDIAMTX_RTSP_PORT}" \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  ${PORT} -> HTTP API http://127.0.0.1:${PORT}/docs
  /media (ro) <- ${MEDIA_DIR}
  /app/video (ro) <- ${VIDEO_DIR}
  /app/output <- ${OUTPUT_DIR}
  /app/log <- ${LOG_DIR}
  MEDIAMTX_HOST=${MEDIAMTX_HOST} MEDIAMTX_RTSP_PORT=${MEDIAMTX_RTSP_PORT}

Prerequisites:
  bash demo/4_ffmpeg/1_build_image.sh
  bash demo/1_mediamtx/1_build_container.sh

Smoke test:
  curl http://127.0.0.1:${PORT}/ffmpeg/hello_world

Stop:
  docker rm -f ${NAME}
EOF
