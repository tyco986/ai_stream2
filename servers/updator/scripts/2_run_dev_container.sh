#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
NAME="${PROJECT_NAME}_updator"
CONTEXT="${ROOT}/servers/updator"

mkdir -p "${ROOT}/logs/updator" "${CONTEXT}/output"
docker network create "${PROJECT_NAME}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}" \
  -p 8095:8095 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${CONTEXT}/output:/root/updator" \
  -v "${ROOT}/logs:/root/logs" \
  "${NAME}"
echo "Updator API: http://127.0.0.1:8095/${PROJECT_NAME}/updator/health"
