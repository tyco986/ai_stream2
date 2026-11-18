#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PROJECT_NAME=ai_stream2
NAME="${NETDATA_CONTAINER_NAME:-${PROJECT_NAME}_netdata}"
docker rm -f "${NAME}" 2>/dev/null || true
echo "Stopped ${NAME}"
