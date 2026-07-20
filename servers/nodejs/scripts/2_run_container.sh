#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

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
  -e PROJECT_ENV_FILE=/project.env \
  -v "${ROOT}/servers/nodejs:/app" \
  -v "${ROOT}/project.env:/project.env:ro" \
  -v "${PROJECT_NAME}_nodejs_modules:/app/node_modules" \
  -v "${ROOT}/logs/nodejs:/root/logs/nodejs" \
  "${PROJECT_NAME}_nodejs" \
  sh -c 'npm run dev -- --host "$HOST" --port "$PORT"'

echo "Debug UI: http://127.0.0.1:5173/"
