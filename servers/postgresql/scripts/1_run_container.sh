#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

NAME="${PROJECT_NAME}_postgresql"
IMAGE="${POSTGRES_IMAGE:-postgres:15}"
VOLUME="${PROJECT_NAME}_postgresql_data"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker volume create "${VOLUME}" >/dev/null
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  -e POSTGRES_DB="${POSTGRES_DB:-${PROJECT_NAME}}" \
  -e POSTGRES_USER="${POSTGRES_USER:-${PROJECT_NAME}}" \
  -e POSTGRES_PASSWORD="${POSTGRES_PASSWORD:-${PROJECT_NAME}}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOLUME}:/var/lib/postgresql/data" \
  -p "${POSTGRES_HOST_PORT:-5432}:5432" \
  "${IMAGE}"

echo "PostgreSQL: ${NAME} on network ${PROJECT_NAME}_default (host port ${POSTGRES_HOST_PORT:-5432}, volume ${VOLUME})"
