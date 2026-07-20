#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

mkdir -p "${ROOT}/models" "${ROOT}/logs"

# Historical container name kept for DeepStream-Yolo tooling; override with DSYOLO_CONTAINER_NAME.
NAME="${DSYOLO_CONTAINER_NAME:-DeepStream-Yolo}"

docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  -p 8090:8090 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/logs:/root/logs" \
  "${PROJECT_NAME}_dsyolo"
echo "DsYolo API: http://127.0.0.1:8090/${PROJECT_NAME}/dsyolo/hello_world"
echo "Swagger:    http://127.0.0.1:8090/docs"
