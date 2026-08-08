#!/usr/bin/env bash
# Run from project root. Start Beszel Hub (web UI).
# Docker Desktop/WSL: publish a host port (default 8093; 8090 is used by export_onnx).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

NAME="${BESZEL_HUB_NAME:-${PROJECT_NAME}_beszel}"
NET="${BESZEL_NETWORK:-${PROJECT_NAME}_default}"
IMG="${BESZEL_HUB_IMAGE:-henrygd/beszel:latest}"
HOST_PORT="${BESZEL_HOST_PORT:-8093}"
VOL_DATA="${PROJECT_NAME}_beszel_data"
VOL_SOCKET="${PROJECT_NAME}_beszel_socket"
APP_URL="${BESZEL_APP_URL:-http://127.0.0.1:${HOST_PORT}}"

docker network create "${NET}" 2>/dev/null || true
docker volume create "${VOL_DATA}" >/dev/null
docker volume create "${VOL_SOCKET}" >/dev/null
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d \
  --name "${NAME}" \
  --network "${NET}" \
  --restart unless-stopped \
  -e "APP_URL=${APP_URL}" \
  -p "${HOST_PORT}:8090" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOL_DATA}:/beszel_data" \
  -v "${VOL_SOCKET}:/beszel_socket" \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  UI: ${APP_URL}
  image: ${IMG}
  Next: open UI → create admin → Add System → copy TOKEN/KEY
        then: bash servers/beszel/scripts/2_run_agent.sh
        In Add System, Host/IP use: /beszel_socket/beszel.sock
EOF
