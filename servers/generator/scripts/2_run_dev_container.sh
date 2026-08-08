#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/generator -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_generator_dev"
mkdir -p "${ROOT}/configs" "${ROOT}/logs" "${ROOT}/attachments"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_generator" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_generator" \
  --network "${PROJECT_NAME}_default" \
  -p 8091:8091 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/configs:/root/configs" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/logs:/root/logs" \
  -v "${ROOT}/servers/generator:/app" \
  "${IMAGE}"

echo "Generator API: http://127.0.0.1:8091/${PROJECT_NAME}/generator/generate"
echo "Swagger:       http://127.0.0.1:8091/docs"
echo "Mode:          dev image=${IMAGE}"
