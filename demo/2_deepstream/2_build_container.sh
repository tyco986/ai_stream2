#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../../../" && pwd)"

NAME="${DEEPSTREAM_CONTAINER_NAME:-ai_stream2_deepstream}"
IMG="${DEEPSTREAM_IMAGE:-ai_stream2_deepstream}"
NET="${DEEPSTREAM_NETWORK:-ai_stream2_default}"
KAFKA="${KAFKA_BROKER:-ai_stream2_kafka:9092}"
PREVIEW_RTP_HOST="${DS_PREVIEW_RTP_HOST:-ai_stream2_mediamtx}"
SSH_PORT="${SSH_HOST_PORT:-2222}"
DNS1="${DOCKER_DNS:-8.8.8.8}"
DNS2="${DOCKER_DNS2:-114.114.114.114}"

docker network create "${NET}" 2>/dev/null || true

echo "=> remove old containers"
for c in "${NAME}" ai_stream2-deepstream-1 deepstream; do
  docker rm -f "${c}" 2>/dev/null || true
done

docker run -d --name "${NAME}" --network "${NET}" --gpus all \
  --dns "${DNS1}" --dns "${DNS2}" \
  -p 9000:9000 -p "${SSH_PORT}:22" \
  -e KAFKA_BROKER="${KAFKA}" \
  -e KAFKA_TOPIC=deepstream-detections \
  -e KAFKA_EVENT_TOPIC=deepstream-events \
  -e KAFKA_COMMAND_TOPIC=deepstream-commands \
  -e DS_PREVIEW_RTP_HOST="${PREVIEW_RTP_HOST}" \
  -v "${ROOT}/deepstream/models:/app/models" \
  -v "${ROOT}/deepstream/config:/app/config" \
  "${IMG}"

cat <<EOF
${NAME} (${NET})  KAFKA_BROKER=${KAFKA}
  9000 -> DeepStream REST API
  preview RTP -> udp://${PREVIEW_RTP_HOST}:5400 (external MediaMTX)
  ${SSH_PORT} -> SSH (root; run install_openssh.sh first)
EOF
