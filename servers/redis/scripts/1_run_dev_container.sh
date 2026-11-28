#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME=ai_stream2
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

mkdir -p "${ROOT}/logs/redis"
docker network create "${PROJECT_NAME}" 2>/dev/null || true
docker volume create "${PROJECT_NAME}_redis" >/dev/null
docker rm -f "${PROJECT_NAME}_redis" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_redis" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}" \
  --entrypoint /bin/sh \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${PROJECT_NAME}_redis:/data" \
  -v "${ROOT}/logs/redis:/logs/redis" \
  -v "${ROOT}/servers/redis/scripts/stream_log.sh:/stream_log.sh:ro" \
  -p 6379:6379 \
  redis:7-alpine \
  -c 'sh /stream_log.sh & exec /usr/local/bin/docker-entrypoint.sh redis-server --appendonly yes --maxmemory 256mb --maxmemory-policy noeviction'

echo "Redis: ${PROJECT_NAME}_redis port 6379"
echo "Log:   ${ROOT}/logs/redis/redis_YYYY-MM-DD.jsonl"
