#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

PROJECT_NAME=ai_stream2
CONTAINER="${PROJECT_NAME}_registry2"
VOLUME="${PROJECT_NAME}_registry2"

docker network create "${PROJECT_NAME}" 2>/dev/null || true
docker volume create "${VOLUME}" >/dev/null
docker rm -f "${CONTAINER}" 2>/dev/null || true

docker run -d \
  --name "${CONTAINER}" \
  --restart unless-stopped \
  --network "${PROJECT_NAME}" \
  -p 5000:5000 \
  -e REGISTRY_STORAGE_DELETE_ENABLED=true \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOLUME}:/var/lib/registry" \
  registry:2

echo "Registry: http://127.0.0.1:5000/v2/"
