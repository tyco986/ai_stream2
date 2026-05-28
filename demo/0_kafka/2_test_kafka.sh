#!/usr/bin/env bash
set -euo pipefail

NAME="${KAFKA_CONTAINER_NAME:-ai_stream2_kafka}"
TOPIC="${KAFKA_TEST_TOPIC:-hello-world}"

echo "=> cluster health"
docker exec "${NAME}" rpk cluster health

echo "=> topic create (ignore if exists)"
docker exec "${NAME}" rpk topic create "${TOPIC}" -p 1 2>/dev/null || true

echo "=> produce"
echo "hello from ${NAME}" | docker exec -i "${NAME}" rpk topic produce "${TOPIC}" -k ping

echo "=> consume"
docker exec "${NAME}" rpk topic consume "${TOPIC}" -n 1

echo "OK"
