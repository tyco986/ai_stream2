#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
mkdir -p "${ROOT}/models" "${ROOT}/logs" "${ROOT}/outputs"

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_exporttrt 2>/dev/null || true
docker run -d \
  --name ai_stream2_exporttrt \
  --network ai_stream2_default \
  --gpus all \
  -p 9000:9000 \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/logs:/root/logs" \
  ai_stream2_exporttrt
echo "Export TRT API: http://127.0.0.1:9000/ai_stream2/exporttrt/export_engine"
echo "Swagger:        http://127.0.0.1:9000/docs"
