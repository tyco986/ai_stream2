#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
NAME="${PROJECT_NAME}_generator"

mkdir -p "${ROOT}/models" "${ROOT}/history/generator" "${ROOT}/logs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  -p 8091:8091 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/history:/root/history" \
  -v "${ROOT}/logs:/root/logs" \
  -v "${ROOT}/attachments:/root/attachments" \
  "${NAME}"
echo "Generator API: http://127.0.0.1:8091/${PROJECT_NAME}/generator/health"
