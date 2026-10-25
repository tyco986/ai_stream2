#!/usr/bin/env bash
# Incremental rebuild inside the running dev container, then restart the process.
set -euo pipefail

PROJECT_NAME=ai_stream2
CONTAINER="${PROJECT_NAME}_backend"

docker exec "${CONTAINER}" cmake --build /tmp/build
docker restart "${CONTAINER}"
echo "Restarted ${CONTAINER}"
