#!/usr/bin/env bash
# Run from project root. Prod image (code baked in; no /app bind mount).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_deepstream_prod"
mkdir -p "${ROOT}/models" "${ROOT}/configs" "${ROOT}/logs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_deepstream" 2>/dev/null || true

docker run \
  -d \
  --name "${PROJECT_NAME}_deepstream" \
  --network "${PROJECT_NAME}_default" \
  --gpus all \
  -p 8092:8092 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -e KAFKA_TOPIC=deepstream-detections \
  -e KAFKA_EVENT_TOPIC=deepstream-events \
  -e KAFKA_COMMAND_TOPIC=deepstream-commands \
  -e DS_PREVIEW_RTP_HOST="${PROJECT_NAME}_mediamtx" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/configs:/root/configs" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/logs:/root/logs" \
  "${IMAGE}"

echo "DeepStream API: http://127.0.0.1:8092/${PROJECT_NAME}/deepstream/start_pipeline"
echo "Swagger:        http://127.0.0.1:8092/docs"
echo "Mode:           prod image=${IMAGE}"
