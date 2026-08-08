#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
NAME="${DOCKER_SOCKET_PROXY_NAME:-${PROJECT_NAME}_docker_socket_proxy}"
NET="${DOCKER_SOCKET_PROXY_NETWORK:-${PROJECT_NAME}_default}"
IMG="${DOCKER_SOCKET_PROXY_IMAGE:-tecnativa/docker-socket-proxy:latest}"
HOST_PORT="${DOCKER_SOCKET_PROXY_HOST_PORT:-127.0.0.1:2375}"

docker network create "${NET}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d \
  --name "${NAME}" \
  --network "${NET}" \
  --restart unless-stopped \
  -v /etc/localtime:/etc/localtime:ro \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  -p "${HOST_PORT}:2375" \
  -e LOG_LEVEL=info \
  -e CONTAINERS=1 \
  -e POST=1 \
  -e ALLOW_START=1 \
  -e ALLOW_STOP=1 \
  -e ALLOW_RESTARTS=1 \
  -e LOGS=1 \
  -e NETWORKS=1 \
  -e IMAGES=1 \
  -e INFO=1 \
  -e VOLUMES=1 \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  in-network -> DOCKER_HOST=tcp://${NAME}:2375
  host         -> DOCKER_HOST=tcp://${HOST_PORT}
EOF
