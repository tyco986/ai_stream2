#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
mkdir -p "${ROOT}/models" "${ROOT}/logs"

docker rm -f DeepStream-Yolo 2>/dev/null || true
docker run -d \
  --name DeepStream-Yolo \
  -p 8090:8090 \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/logs:/root/logs" \
  ai_stream2_dsyolo
echo "DsYolo API: http://127.0.0.1:8090/ai_stream2/dsyolo/hello_world"
echo "Swagger:    http://127.0.0.1:8090/docs"
