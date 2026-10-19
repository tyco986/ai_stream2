#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

PROJECT_NAME=ai_stream2

IMAGE="${PROJECT_NAME}_frontend_dev"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_frontend" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_frontend" \
  --network "${PROJECT_NAME}_default" \
  -p 80:80 \
  -v /etc/localtime:/etc/localtime:ro \
  "${IMAGE}"

echo "Frontend: http://127.0.0.1/"
echo "Mode:     dev image=${IMAGE}"
