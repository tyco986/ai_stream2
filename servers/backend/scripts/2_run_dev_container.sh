#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/backend -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_backend_dev"
mkdir -p "${ROOT}/logs/backend"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_backend" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_backend" \
  --network "${PROJECT_NAME}_default" \
  -p "${BACKEND_HOST_PORT:-8001}:8000" \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -e PROJECT_ENV_FILE=/project.env \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/servers/backend:/app" \
  -v "${ROOT}/project.env:/project.env:ro" \
  -v "${ROOT}/logs/backend:/root/logs/backend" \
  "${IMAGE}" \
  sh -c 'cmake -S /app -B /tmp/build && cmake --build /tmp/build && /tmp/build/backend'

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
echo "Backend API: http://127.0.0.1:${HOST_PORT}/health"
echo "Mode:        dev image=${IMAGE}"
