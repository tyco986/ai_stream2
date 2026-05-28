#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

NAME="${MEDIAMTX_CONTAINER_NAME:-ai_stream2_mediamtx}"
NET="${MEDIAMTX_NETWORK:-ai_stream2_default}"
IMG="${MEDIAMTX_IMAGE:-bluenviron/mediamtx:1.17.1}"
CFG="${MEDIAMTX_CONFIG:-${ROOT}/mediamtx/mediamtx.yml}"
RTSP_PORT="${MEDIAMTX_RTSP_PORT:-8554}"
WEBRTC_PORT="${MEDIAMTX_WEBRTC_PORT:-8889}"
RTP_PORT="${MEDIAMTX_RTP_PORT:-5400}"

docker network create "${NET}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d --name "${NAME}" --network "${NET}" \
  -p "${RTSP_PORT}:8554" -p "${WEBRTC_PORT}:8889" -p "${RTP_PORT}:5400/udp" \
  -v "${CFG}:/mediamtx.yml:ro" \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  ${RTSP_PORT} -> RTSP (publish/read, e.g. rtsp://${NAME}:8554/cam1)
  ${WEBRTC_PORT} -> WebRTC preview http://127.0.0.1:${WEBRTC_PORT}/preview/
  ${RTP_PORT}/udp -> preview RTP from DeepStream (point udpsink at ${NAME}:${RTP_PORT})
EOF
