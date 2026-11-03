#!/usr/bin/env bash
# Incremental rebuild inside the running container, then restart the process.
set -euo pipefail

PROJECT_NAME=ai_stream2
CONTAINER="${PROJECT_NAME}_generator"

docker exec "${CONTAINER}" bash -c 'cmake --build /tmp/build && cp /tmp/build/generator /app/generator'
docker restart "${CONTAINER}"
echo "Restarted ${CONTAINER}"
