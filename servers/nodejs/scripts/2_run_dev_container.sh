#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/nodejs -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_nodejs_dev"
mkdir -p "${ROOT}/logs/nodejs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_nodejs" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_nodejs" \
  --network "${PROJECT_NAME}_default" \
  -p 5173:5173 \
  -p 5174:5174 \
  -e HOST=0.0.0.0 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -e VITE_PROJECT_NAME="${PROJECT_NAME}" \
  -e VITE_BACKEND_PROXY_TARGET="${VITE_BACKEND_PROXY_TARGET:-http://${PROJECT_NAME}_django:8000}" \
  -e PROJECT_ENV_FILE=/project.env \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/servers/nodejs:/app" \
  -v "${ROOT}/project.env:/project.env:ro" \
  -v "${PROJECT_NAME}_nodejs_modules:/app/node_modules" \
  -v "${ROOT}/logs/nodejs:/root/logs/nodejs" \
  "${IMAGE}" \
  sh -c 'chmod +x scripts/dev-both.sh && npm run dev'

echo "UI real: http://127.0.0.1:5173/"
echo "UI mock: http://127.0.0.1:5174/"
echo "Proxy → ${VITE_BACKEND_PROXY_TARGET:-http://${PROJECT_NAME}_django:8000}"
echo "Mode:   dev image=${IMAGE}"
