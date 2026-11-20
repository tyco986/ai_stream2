#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME=ai_stream2

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker volume create "${PROJECT_NAME}_redis_data" >/dev/null
docker rm -f "${PROJECT_NAME}_redis" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_redis" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}_default" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${PROJECT_NAME}_redis_data:/data" \
  -p 6379:6379 \
  redis:7-alpine \
  redis-server --appendonly yes --maxmemory 256mb --maxmemory-policy noeviction

echo "Redis: ${PROJECT_NAME}_redis port 6379"
