#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

mkdir -p "${ROOT}/recordings" "${ROOT}/outputs" "${ROOT}/logs/ffmpeg"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_ffmpeg" 2>/dev/null || true
docker run -d \
  --name "${PROJECT_NAME}_ffmpeg" \
  --network "${PROJECT_NAME}_default" \
  -p 8080:8080 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v "${ROOT}/recordings:/root/recordings" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/logs:/root/logs" \
  "${PROJECT_NAME}_ffmpeg"
echo "FFmpeg API: http://127.0.0.1:8080/${PROJECT_NAME}/ffmpeg/hello_world"
echo "Swagger:    http://127.0.0.1:8080/docs"
