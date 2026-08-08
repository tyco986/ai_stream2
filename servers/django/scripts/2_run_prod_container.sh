#!/usr/bin/env bash
# Run from project root. Prod image (code baked in; no /app bind mount).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_django_prod"
mkdir -p "${ROOT}/logs/django" "${ROOT}/logs/models_builds" "${ROOT}/secrets/age/payloads" "${ROOT}/models" "${ROOT}/media/events" "${ROOT}/recordings"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_django" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_django" \
  --network "${PROJECT_NAME}_default" \
  -p "${DJANGO_HOST_PORT:-8000}:8000" \
  -e HOST=0.0.0.0 \
  -e PORT=8000 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -e PROJECT_ENV_FILE=/project.env \
  -e DEBUG=0 \
  -e DJANGO_SECRET_KEY="${DJANGO_SECRET_KEY:?DJANGO_SECRET_KEY is required for prod}" \
  -e POSTGRES_HOST="${POSTGRES_HOST:-${PROJECT_NAME}_postgresql}" \
  -e POSTGRES_PORT="${POSTGRES_PORT:-5432}" \
  -e POSTGRES_DB="${POSTGRES_DB:-${PROJECT_NAME}}" \
  -e POSTGRES_USER="${POSTGRES_USER:-${PROJECT_NAME}}" \
  -e POSTGRES_PASSWORD="${POSTGRES_PASSWORD:?POSTGRES_PASSWORD is required for prod}" \
  -e AGE_DEV_PUB_PATH=/app/keys/dev.pub \
  -e TICKET_PUB_PATH=/app/keys/ticket.pub \
  -e AGE_SITE_KEY_PATH=/secrets/age/site.key \
  -e AGE_SITE_PUB_PATH=/secrets/age/site.pub \
  -e SITE_CONFIG_PAYLOAD_DIR=/secrets/age/payloads \
  -e EVENTS_MEDIA_DIR=/root/media/events \
  -e RECORDINGS_ROOT=/recordings \
  -e PUBLIC_API_ORIGIN="http://127.0.0.1:${DJANGO_HOST_PORT:-8000}" \
  -e MEDIAMTX_RECORD_ROOT=/recordings \
  -e RUN_MIGRATE=1 \
  -e HOST_PROJECT_ROOT="${ROOT}" \
  -e DOCKER_HOST="tcp://${PROJECT_NAME}_docker_socket_proxy:2375" \
  -e DEEPSTREAM_IMAGE="${DEEPSTREAM_IMAGE:-${PROJECT_NAME}_deepstream_prod}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/project.env:/project.env:ro" \
  -v "${ROOT}/logs/django:/root/logs/django" \
  -v "${ROOT}/logs/models_builds:/app/logs/models_builds" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/configs:/root/configs" \
  -v "${ROOT}/media/events:/root/media/events" \
  -v "${ROOT}/recordings:/recordings" \
  -v "${ROOT}/secrets/age:/secrets/age" \
  "${IMAGE}" \
  gunicorn config.wsgi:application --bind 0.0.0.0:8000

HOST_PORT="${DJANGO_HOST_PORT:-8000}"
echo "Django API: http://127.0.0.1:${HOST_PORT}/${PROJECT_NAME}/backend/"
echo "Shell me:   http://127.0.0.1:${HOST_PORT}/${PROJECT_NAME}/backend/shell/me"
echo "Mode:       prod image=${IMAGE}"
