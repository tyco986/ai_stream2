#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
mkdir -p "${ROOT}/recordings" "${ROOT}/outputs" "${ROOT}/logs/ffmpeg"

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_ffmpeg 2>/dev/null || true
docker run -d \
  --name ai_stream2_ffmpeg \
  --network ai_stream2_default \
  -p 8080:8080 \
  -v "${ROOT}/recordings:/root/recordings" \
  -v "${ROOT}/outputs:/root/outputs" \
  -v "${ROOT}/logs:/root/logs" \
  ai_stream2_ffmpeg
echo "FFmpeg API: http://127.0.0.1:8080/ai_stream2/ffmpeg/hello_world"
echo "Swagger:    http://127.0.0.1:8080/docs"
