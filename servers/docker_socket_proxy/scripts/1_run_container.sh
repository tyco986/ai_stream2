#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

PROJECT_NAME=ai_stream2
mkdir -p "${ROOT}/logs/docker_socket_proxy"
NAME="${DOCKER_SOCKET_PROXY_NAME:-${PROJECT_NAME}_docker_socket_proxy}"
NET="${DOCKER_SOCKET_PROXY_NETWORK:-${PROJECT_NAME}_default}"
IMG="${DOCKER_SOCKET_PROXY_IMAGE:-tecnativa/docker-socket-proxy:latest}"
HOST_PORT="${DOCKER_SOCKET_PROXY_HOST_PORT:-127.0.0.1:2375}"

docker network create "${NET}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d \
  --name "${NAME}" \
  --ulimit core=0 \
  --network "${NET}" \
  --restart unless-stopped \
  -v /etc/localtime:/etc/localtime:ro \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  -v "${ROOT}/logs/docker_socket_proxy:/logs/docker_socket_proxy" \
  -p "${HOST_PORT}:2375" \
  --entrypoint sh \
  -e LOG_LEVEL=debug \
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
  "${IMG}" \
  -c 'sed -i "s|\${LOG_LEVEL}|debug|g" /usr/local/etc/haproxy/haproxy.cfg.template && exec /usr/local/bin/docker-entrypoint.sh haproxy -f /tmp/haproxy.cfg >> /logs/docker_socket_proxy/docker_socket_proxy.log 2>&1'

cat <<EOF
${NAME} (${NET})
  in-network -> DOCKER_HOST=tcp://${NAME}:2375
  host         -> DOCKER_HOST=tcp://${HOST_PORT}
EOF
