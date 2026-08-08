#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

mkdir -p "${ROOT}/models" "${ROOT}/logs"

NAME="${EXPORT_ONNX_CONTAINER_NAME:-${PROJECT_NAME}_export_onnx}"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  -p 8090:8090 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/logs:/root/logs" \
  "${PROJECT_NAME}_export_onnx"
echo "ExportOnnx API: http://127.0.0.1:8090/${PROJECT_NAME}/export_onnx/health"
echo "Swagger:    http://127.0.0.1:8090/docs"
