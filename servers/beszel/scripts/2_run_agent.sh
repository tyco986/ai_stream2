#!/usr/bin/env bash
# Run from project root. Start Beszel Agent (same host as Hub via shared unix socket).
# Requires TOKEN/KEY from Hub UI (Add System), or universal token from Settings → Tokens.
#
#   export BESZEL_TOKEN='...'
#   export BESZEL_KEY='ssh-ed25519 ...'
#   bash servers/beszel/scripts/2_run_agent.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

NAME="${BESZEL_AGENT_NAME:-${PROJECT_NAME}_beszel_agent}"
HUB_NAME="${BESZEL_HUB_NAME:-${PROJECT_NAME}_beszel}"
NET="${BESZEL_NETWORK:-${PROJECT_NAME}_default}"
IMG="${BESZEL_AGENT_IMAGE:-henrygd/beszel-agent:latest}"
VOL_AGENT="${PROJECT_NAME}_beszel_agent_data"
VOL_SOCKET="${PROJECT_NAME}_beszel_socket"
HUB_URL="${BESZEL_HUB_URL:-http://${HUB_NAME}:8090}"
TOKEN="${BESZEL_TOKEN:-}"
KEY="${BESZEL_KEY:-}"

if [[ -z "${TOKEN}" || -z "${KEY}" ]]; then
  echo "BESZEL_TOKEN and BESZEL_KEY are required (from Hub Add System / Settings → Tokens)." >&2
  exit 1
fi

docker network create "${NET}" 2>/dev/null || true
docker volume create "${VOL_AGENT}" >/dev/null
docker volume create "${VOL_SOCKET}" >/dev/null
docker rm -f "${NAME}" 2>/dev/null || true

GPU_ARGS=()
if command -v nvidia-smi >/dev/null 2>&1; then
  GPU_ARGS=(--gpus "all,capabilities=utility")
fi

docker run -d \
  --name "${NAME}" \
  --network "${NET}" \
  --restart unless-stopped \
  "${GPU_ARGS[@]}" \
  -e "LISTEN=/beszel_socket/beszel.sock" \
  -e "HUB_URL=${HUB_URL}" \
  -e "TOKEN=${TOKEN}" \
  -e "KEY=${KEY}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOL_AGENT}:/var/lib/beszel-agent" \
  -v "${VOL_SOCKET}:/beszel_socket" \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  hub: ${HUB_URL}
  listen: /beszel_socket/beszel.sock
  image: ${IMG}
  gpu: $([ ${#GPU_ARGS[@]} -gt 0 ] && echo enabled || echo disabled)
EOF
