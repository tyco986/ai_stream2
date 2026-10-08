#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME=ai_stream2

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker volume create "${PROJECT_NAME}_postgresql_data" >/dev/null
docker rm -f "${PROJECT_NAME}_postgresql" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_postgresql" \
  --network "${PROJECT_NAME}_default" \
  -e POSTGRES_DB="${PROJECT_NAME}" \
  -e POSTGRES_USER="${PROJECT_NAME}" \
  -e POSTGRES_PASSWORD="${PROJECT_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${PROJECT_NAME}_postgresql_data:/var/lib/postgresql/data" \
  -p 5432:5432 \
  postgres:15

echo "PostgreSQL: ${PROJECT_NAME}_postgresql port 5432"
