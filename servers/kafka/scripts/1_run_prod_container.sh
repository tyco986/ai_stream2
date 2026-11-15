#!/usr/bin/env bash
# Run from project root. Redpanda on the project network with the image entrypoint and command.
set -euo pipefail

PROJECT_NAME=ai_stream2

IMAGE="docker.redpanda.com/redpandadata/redpanda:v24.1.9"
HOST_PORT="${KAFKA_HOST_PORT:-19092}"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker volume create "${PROJECT_NAME}_kafka_data" >/dev/null
docker run --rm --user 0 \
  -v "${PROJECT_NAME}_kafka_data:/var/lib/redpanda/data" \
  --entrypoint /bin/bash \
  "${IMAGE}" \
  -c 'mkdir -p /var/lib/redpanda/data && chown -R redpanda:redpanda /var/lib/redpanda/data'
docker rm -f "${PROJECT_NAME}_kafka" 2>/dev/null || true

echo "Kafka API: 127.0.0.1:${HOST_PORT}"
echo "Mode:      prod image=${IMAGE}"

docker run -d \
  --name "${PROJECT_NAME}_kafka" \
  --ulimit core=0 \
  --network "${PROJECT_NAME}_default" \
  -p "${HOST_PORT}:9092" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${PROJECT_NAME}_kafka_data:/var/lib/redpanda/data" \
  "${IMAGE}"
