#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
NAME="${PROJECT_NAME}_export_trt"

mkdir -p "${ROOT}/models" "${ROOT}/history" "${ROOT}/logs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  --gpus all \
  -p 8080:8080 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/history:/root/history" \
  -v "${ROOT}/logs:/root/logs" \
  "${NAME}"
echo "http://127.0.0.1:8080/${PROJECT_NAME}/export_trt/health"
