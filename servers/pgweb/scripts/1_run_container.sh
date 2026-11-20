#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME=ai_stream2

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_pgweb" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_pgweb" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}_default" \
  -p 8081:8081 \
  sosedoff/pgweb \
  --url "postgres://${PROJECT_NAME}:${PROJECT_NAME}@${PROJECT_NAME}_postgresql:5432/${PROJECT_NAME}?sslmode=disable"

echo "pgweb: http://127.0.0.1:8081"
