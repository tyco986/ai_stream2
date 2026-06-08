#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"
mkdir -p outputs/ffmpeg logs/ffmpeg attachments/videos recordings

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_ffmpeg 2>/dev/null || true

docker run -d --name ai_stream2_ffmpeg --network ai_stream2_default \
  -p 8080:8080 \
  -v "${ROOT}/outputs/ffmpeg:/app/output" \
  -v "${ROOT}/logs/ffmpeg:/app/log" \
  -v "${ROOT}/recordings:/recordings:ro" \
  -v "${ROOT}/attachments/videos:/app/video:ro" \
  -e MEDIAMTX_HOST=ai_stream2_mediamtx \
  -e MEDIAMTX_RTSP_PORT=8554 \
  ai_stream2_ffmpeg

cat <<EOF
ai_stream2_ffmpeg (ai_stream2_default)
  8080 -> HTTP API http://127.0.0.1:8080
  8080 -> Swagger http://127.0.0.1:8080/docs
EOF
