#!/usr/bin/env bash
# Incremental rebuild inside the running container, then restart the process.
set -euo pipefail

PROJECT_NAME=ai_stream2
CONTAINER="${PROJECT_NAME}_export_trt"

docker exec "${CONTAINER}" bash -c 'cmake --build /tmp/build && cp /tmp/build/export_trt /app/export_trt'
docker restart "${CONTAINER}"
echo "Restarted ${CONTAINER}"
