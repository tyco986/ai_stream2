#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
mkdir -p "${ROOT}/models" "${ROOT}/configs" "${ROOT}/logs"

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_deepstream 2>/dev/null || true

docker run -d \
  --name ai_stream2_deepstream \
  --network ai_stream2_default \
  --gpus all \
  -p 8092:8092 \
  -e KAFKA_BROKER=ai_stream2_kafka:9092 \
  -e KAFKA_TOPIC=deepstream-detections \
  -e KAFKA_EVENT_TOPIC=deepstream-events \
  -e KAFKA_COMMAND_TOPIC=deepstream-commands \
  -e DS_PREVIEW_RTP_HOST=ai_stream2_mediamtx \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/configs:/root/configs" \
  -v "${ROOT}/attachments:/root/attachments" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/logs:/root/logs" \
  ai_stream2_deepstream

echo "DeepStream API: http://127.0.0.1:8092/ai_stream2/deepstream/build_pipeline"
echo "Swagger:        http://127.0.0.1:8092/docs"
