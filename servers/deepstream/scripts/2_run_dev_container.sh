#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/deepstream -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
NAME="${PROJECT_NAME}_deepstream"
IMAGE="${PROJECT_NAME}_deepstream_dev"

mkdir -p "${ROOT}/models" "${ROOT}/history" "${ROOT}/logs" "${ROOT}/outputs" "${ROOT}/attachments" "${ROOT}/media"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

echo "DeepStream API: http://127.0.0.1:8094/${PROJECT_NAME}/deepstream/health"
echo "Mode:           dev image=${IMAGE}"

docker run \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  --gpus all \
  --ulimit core=0 \
  -e NVDS_ENABLE_LATENCY_MEASUREMENT=1 \
  -e NVDS_ENABLE_COMPONENT_LATENCY_MEASUREMENT=1 \
  -e GST_DEBUG=3 \
  -p 8094:8094 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/servers/deepstream:/app" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/history:/root/history" \
  -v "${ROOT}/logs:/root/logs" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/media:/root/media" \
  -e GST_REGISTRY=/tmp/gst-registry.bin \
  "${IMAGE}" \
  bash /app/scripts/5_dev_start.sh
