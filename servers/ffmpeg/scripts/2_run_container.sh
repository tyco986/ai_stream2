#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

IMAGE="${PROJECT_NAME}_ffmpeg"
NAME="${PROJECT_NAME}_ffmpeg"

mkdir -p "${ROOT}/media"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  -p 8080:8080 \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/media:/root/media" \
  "${IMAGE}"

echo "FFmpeg API: http://127.0.0.1:8080/health"
