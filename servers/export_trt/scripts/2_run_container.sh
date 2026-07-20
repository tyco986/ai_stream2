#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

mkdir -p "${ROOT}/models" "${ROOT}/logs" "${ROOT}/outputs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_export_trt" 2>/dev/null || true
docker run -d \
  --name "${PROJECT_NAME}_export_trt" \
  --network "${PROJECT_NAME}_default" \
  --gpus all \
  -p 9000:9000 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/logs:/root/logs" \
  "${PROJECT_NAME}_export_trt"
echo "Export TRT API: http://127.0.0.1:9000/${PROJECT_NAME}/export_trt/export_engine"
echo "Swagger:        http://127.0.0.1:9000/docs"
