#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

HUB_NAME="${BESZEL_HUB_NAME:-${PROJECT_NAME}_beszel}"
AGENT_NAME="${BESZEL_AGENT_NAME:-${PROJECT_NAME}_beszel_agent}"

docker rm -f "${AGENT_NAME}" 2>/dev/null || true
docker rm -f "${HUB_NAME}" 2>/dev/null || true
echo "Stopped ${AGENT_NAME} ${HUB_NAME}"
