#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
cd "$ROOT"
mkdir -p recordings logs/mediamtx

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_mediamtx" 2>/dev/null || true

docker run -d --name "${PROJECT_NAME}_mediamtx" --network "${PROJECT_NAME}_default" \
  -p 8554:8554 -p 8889:8889 -p 9996:9996 -p 9997:9997 \
  -p 8189:8189/udp -p 8189:8189/tcp \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/configs/mediamtx/mediamtx.yml:/mediamtx.yml:ro" \
  -v "${ROOT}/logs/mediamtx:/logs/mediamtx" \
  -v "${ROOT}/recordings:/recordings" \
  bluenviron/mediamtx:1.17.1

cat <<EOF
${PROJECT_NAME}_mediamtx (${PROJECT_NAME}_default)
  8554 -> RTSP publish/read (e.g. rtsp://${PROJECT_NAME}_mediamtx:8554/video1_B0)
  8889 -> WebRTC http://127.0.0.1:8889/<path>/
  8189 -> WebRTC ICE (UDP/TCP media)
  9996 -> Playback http://127.0.0.1:9996/get?path=&start=&duration=
  9997 -> Control API http://127.0.0.1:9997/v3/paths/list
       (in-network) http://${PROJECT_NAME}_mediamtx:9997/v3/...
EOF
