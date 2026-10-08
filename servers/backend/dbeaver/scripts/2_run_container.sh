#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME=ai_stream2

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_cloudbeaver" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_cloudbeaver" \
  --network "${PROJECT_NAME}_default" \
  -p 8978:8978 \
  dbeaver/cloudbeaver

echo "CloudBeaver: http://127.0.0.1:8978"
