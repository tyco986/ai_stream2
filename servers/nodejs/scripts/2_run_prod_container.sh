#!/usr/bin/env bash
# Run from project root. Prod image (built assets; no /app bind mount).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_nodejs_prod"
mkdir -p "${ROOT}/logs/nodejs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_nodejs" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_nodejs" \
  --network "${PROJECT_NAME}_default" \
  -p 5173:5173 \
  -e HOST=0.0.0.0 \
  -e PORT=5173 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -e VITE_PROJECT_NAME="${PROJECT_NAME}" \
  -e VITE_BACKEND_PROXY_TARGET="${VITE_BACKEND_PROXY_TARGET:-http://${PROJECT_NAME}_django:8000}" \
  -e PROJECT_ENV_FILE=/project.env \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/project.env:/project.env:ro" \
  -v "${ROOT}/logs/nodejs:/root/logs/nodejs" \
  "${IMAGE}"

echo "UI real: http://127.0.0.1:5173/"
echo "Proxy → ${VITE_BACKEND_PROXY_TARGET:-http://${PROJECT_NAME}_django:8000}"
echo "Mode:   prod image=${IMAGE}"
