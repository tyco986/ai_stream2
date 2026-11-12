#!/usr/bin/env bash
# Run from project root. Prod image (code baked in; no source bind mount).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
NAME="${PROJECT_NAME}_deepstream"
IMAGE="${PROJECT_NAME}_deepstream"

mkdir -p "${ROOT}/models" "${ROOT}/history" "${ROOT}/logs" "${ROOT}/outputs" "${ROOT}/attachments" "${ROOT}/media"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d \
  --name "${NAME}" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}_default" \
  --gpus all \
  -p 8094:8094 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/history:/root/history" \
  -v "${ROOT}/logs:/root/logs" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/media:/root/media" \
  "${IMAGE}"

echo "DeepStream API: http://127.0.0.1:8094/${PROJECT_NAME}/deepstream/health"
echo "Mode:           prod image=${IMAGE}"
