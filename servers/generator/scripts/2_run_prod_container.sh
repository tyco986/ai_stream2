#!/usr/bin/env bash
# Run from project root. Prod image (code baked in; no /app bind mount).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
IMAGE="ai_stream2_generator_prod"
mkdir -p "${ROOT}/configs" "${ROOT}/logs" "${ROOT}/attachments"

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_generator 2>/dev/null || true

docker run -d \
  --name ai_stream2_generator \
  --network ai_stream2_default \
  -p 8091:8091 \
  -v "${ROOT}/configs:/root/configs" \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/logs:/root/logs" \
  "${IMAGE}"

echo "Generator API: http://127.0.0.1:8091/ai_stream2/generator/generate"
echo "Swagger:       http://127.0.0.1:8091/docs"
echo "Mode:          prod image=${IMAGE}"
