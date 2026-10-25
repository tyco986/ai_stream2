#!/usr/bin/env bash
# Run from project root. Dev image + mount pure-admin-thin-i18n -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2
IMAGE="${PROJECT_NAME}_frontend_dev"
APP="${ROOT}/servers/frontend/pure-admin-thin-i18n"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_frontend" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_frontend" \
  --network "${PROJECT_NAME}_default" \
  -p 8848:8848 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${APP}:/app" \
  -v /app/node_modules \
  "${IMAGE}"

echo "Frontend: http://127.0.0.1:8848"
echo "Mode:     dev image=${IMAGE}"
