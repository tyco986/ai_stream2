#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2
CONTAINER="${PROJECT_NAME}_registry2"
DATA="${ROOT}/servers/registry2/data"

mkdir -p "${DATA}"
docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${CONTAINER}" 2>/dev/null || true

docker run -d \
  --name "${CONTAINER}" \
  --restart unless-stopped \
  --network "${PROJECT_NAME}_default" \
  -p 5000:5000 \
  -e REGISTRY_STORAGE_DELETE_ENABLED=true \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${DATA}:/var/lib/registry" \
  registry:2

echo "Registry: http://127.0.0.1:5000/v2/"
