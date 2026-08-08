#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

NAME="${KAFKA_CONTAINER_NAME:-${PROJECT_NAME}_kafka}"
TOPIC="${KAFKA_TEST_TOPIC:-health-check}"

echo "=> cluster health"
docker exec "${NAME}" rpk cluster health

echo "=> topic create (ignore if exists)"
docker exec "${NAME}" rpk topic create "${TOPIC}" -p 1 2>/dev/null || true

echo "=> produce"
echo "hello from ${NAME}" | docker exec -i "${NAME}" rpk topic produce "${TOPIC}" -k ping

echo "=> consume"
docker exec "${NAME}" rpk topic consume "${TOPIC}" -n 1

echo "OK"
