#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"

mkdir -p "${ROOT}/history" "${ROOT}/logs"

NAME="${PROJECT_NAME}_export_onnx"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  -p 8090:8090 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/history:/root/history" \
  -v "${ROOT}/logs:/root/logs" \
  "${PROJECT_NAME}_export_onnx"
echo "ExportOnnx API: http://127.0.0.1:8090/${PROJECT_NAME}/export_onnx/health"
echo "Swagger:    http://127.0.0.1:8090/docs"
