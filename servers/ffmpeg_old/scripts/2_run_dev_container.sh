#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/ffmpeg -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_ffmpeg_dev"
mkdir -p "${ROOT}/recordings" "${ROOT}/outputs" "${ROOT}/logs/ffmpeg"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_ffmpeg" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_ffmpeg" \
  --network "${PROJECT_NAME}_default" \
  -p 8080:8080 \
  -e HOST=0.0.0.0 \
  -e PORT=8080 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -e PROJECT_ENV_FILE=/project.env \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/servers/ffmpeg:/app" \
  -v "${ROOT}/project.env:/project.env:ro" \
  -v "${ROOT}/recordings:/root/recordings" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/logs:/root/logs" \
  "${IMAGE}" \
  sh -c 'python main.py --host 0.0.0.0 --port 8080'

echo "FFmpeg API: http://127.0.0.1:8080/${PROJECT_NAME}/ffmpeg/health"
echo "Swagger:    http://127.0.0.1:8080/docs"
echo "Mode:       dev image=${IMAGE}"
