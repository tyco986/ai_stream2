

#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/backend -> /src.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

IMAGE="${PROJECT_NAME}_backend_dev"
mkdir -p "${ROOT}/logs/backend" "${ROOT}/media" "${ROOT}/events" "${ROOT}/models" "${ROOT}/configs"

docker network create "${PROJECT_NAME}" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_backend" 2>/dev/null || true

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
echo "Backend API: http://127.0.0.1:${HOST_PORT}/health"
echo "Mode:        dev image=${IMAGE}"

docker run -d \
  --name "${PROJECT_NAME}_backend" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}" \
  -p "${HOST_PORT}:8000" \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/servers/backend:/src" \
  -v "${ROOT}/servers/backend/locales:/app/locales" \
  -v "${ROOT}/servers/backend/src/db/migrations:/app/migrations" \
  -v "${ROOT}/logs:/root/logs" \
  -v "${ROOT}/media:/root/media" \
  -v "${ROOT}/events:/root/events" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/configs:/root/configs" \
  "${IMAGE}" \
  bash /src/scripts/4_dev_start.sh
