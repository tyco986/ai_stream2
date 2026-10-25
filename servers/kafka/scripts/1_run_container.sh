#!/usr/bin/env bash
# Run from project root. Redpanda dev container on the project network.
set -euo pipefail

PROJECT_NAME=ai_stream2

IMAGE="docker.redpanda.com/redpandadata/redpanda:v24.1.9"
HOST_PORT="${KAFKA_HOST_PORT:-19092}"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_kafka" 2>/dev/null || true

echo "Kafka API: 127.0.0.1:${HOST_PORT}"
echo "Mode:      dev image=${IMAGE}"

docker run -d \
  --name "${PROJECT_NAME}_kafka" \
  --network "${PROJECT_NAME}_default" \
  -p "${HOST_PORT}:19092" \
  -v /etc/localtime:/etc/localtime:ro \
  "${IMAGE}" \
  redpanda start \
  --kafka-addr internal://0.0.0.0:9092,external://0.0.0.0:19092 \
  --advertise-kafka-addr "internal://${PROJECT_NAME}_kafka:9092,external://localhost:${HOST_PORT}" \
  --mode dev-container --smp 1
