#!/usr/bin/env bash
set -euo pipefail

NAME="${KAFKA_CONTAINER_NAME:-ai_stream2_kafka}"
NET="${KAFKA_NETWORK:-ai_stream2_default}"
IMG="${KAFKA_IMAGE:-docker.redpanda.com/redpandadata/redpanda:v24.1.9}"
PORT="${KAFKA_HOST_PORT:-19092}"

docker network create "${NET}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d --name "${NAME}" --network "${NET}" -p "${PORT}:19092" "${IMG}" \
  redpanda start \
  --kafka-addr internal://0.0.0.0:9092,external://0.0.0.0:19092 \
  --advertise-kafka-addr "internal://${NAME}:9092,external://localhost:${PORT}" \
  --mode dev-container --smp 1

cat <<EOF
${NAME} (${NET})
  9092 -> Kafka API (in-network, e.g. ${NAME}:9092)
  ${PORT} -> Kafka API (host / WSL, localhost:${PORT})
EOF
