#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT"
mkdir -p recordings logs/mediamtx

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_mediamtx 2>/dev/null || true

docker run -d --name ai_stream2_mediamtx --network ai_stream2_default \
  -p 8554:8554 -p 8889:8889 -p 9997:9997 \
  -p 8189:8189/udp -p 8189:8189/tcp \
  -v "${ROOT}/configs/mediamtx/mediamtx.yml:/mediamtx.yml:ro" \
  -v "${ROOT}/logs/mediamtx:/logs/mediamtx" \
  -v "${ROOT}/recordings:/recordings" \
  bluenviron/mediamtx:1.17.1

cat <<EOF
ai_stream2_mediamtx (ai_stream2_default)
  8554 -> RTSP publish/read (e.g. rtsp://ai_stream2_mediamtx:8554/video1_B0)
  8889 -> WebRTC http://127.0.0.1:8889/<path>/
  8189 -> WebRTC ICE (UDP/TCP media)
  9997 -> Control API http://127.0.0.1:9997/v3/paths/list
       (in-network) http://ai_stream2_mediamtx:9997/v3/...
EOF
