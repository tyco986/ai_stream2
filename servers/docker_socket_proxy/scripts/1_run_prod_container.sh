#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PROJECT_NAME=ai_stream2
NAME="${PROJECT_NAME}_docker_socket_proxy"

docker compose -f "${ROOT}/docker-compose-control-node.yml" up -d docker_socket_proxy

cat <<EOF
${NAME} (${PROJECT_NAME})
  in-network -> DOCKER_HOST=tcp://${NAME}:2375
  host         -> DOCKER_HOST=tcp://127.0.0.1:2375
EOF
