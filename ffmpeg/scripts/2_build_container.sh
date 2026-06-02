#!/usr/bin/env bash
# Run from project root: bash ffmpeg/scripts/2_build_container.sh
set -euo pipefail

mkdir -p media ffmpeg/output ffmpeg/log

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_ffmpeg 2>/dev/null || true

docker run -d --name ai_stream2_ffmpeg --network ai_stream2_default \
  -p 8080:8080 \
  -v "$(pwd)/ffmpeg/output:/app/output" \
  -v "$(pwd)/ffmpeg/log:/app/log" \
  -v "$(pwd)/media:/media:ro" \
  -v "$(pwd)/ffmpeg/video:/app/video:ro" \
  -e MEDIAMTX_HOST=ai_stream2_mediamtx \
  -e MEDIAMTX_RTSP_PORT=8554 \
  ai_stream2_ffmpeg

cat <<EOF
ai_stream2_ffmpeg (ai_stream2_default)
  8080 -> HTTP API http://127.0.0.1:8080
  8080 -> Swagger http://127.0.0.1:8080/docs
EOF
