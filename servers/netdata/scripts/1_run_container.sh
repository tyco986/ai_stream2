#!/usr/bin/env bash
# Run from project root. Official Netdata Agent (host mounts + Docker sock + optional GPU).
# Note: Do not use --network=host on Docker Desktop/WSL — UI would not reach Windows/WSL localhost.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

NAME="${NETDATA_CONTAINER_NAME:-${PROJECT_NAME}_netdata}"
NET="${NETDATA_NETWORK:-${PROJECT_NAME}_default}"
IMG="${NETDATA_IMAGE:-netdata/netdata:stable}"
HOST_PORT="${NETDATA_HOST_PORT:-19999}"
VOL_CONFIG="${PROJECT_NAME}_netdata_config"
VOL_LIB="${PROJECT_NAME}_netdata_lib"
VOL_CACHE="${PROJECT_NAME}_netdata_cache"

docker network create "${NET}" 2>/dev/null || true
docker volume create "${VOL_CONFIG}" >/dev/null
docker volume create "${VOL_LIB}" >/dev/null
docker volume create "${VOL_CACHE}" >/dev/null
docker rm -f "${NAME}" 2>/dev/null || true

GPU_ARGS=()
if command -v nvidia-smi >/dev/null 2>&1; then
  GPU_ARGS=(--gpus "all,capabilities=utility")
fi

docker run -d \
  --name "${NAME}" \
  --network "${NET}" \
  --pid=host \
  --restart unless-stopped \
  --cap-add SYS_PTRACE \
  --cap-add SYS_ADMIN \
  --security-opt apparmor=unconfined \
  "${GPU_ARGS[@]}" \
  -e DISABLE_TELEMETRY=1 \
  -e NETDATA_DISABLE_CLOUD=1 \
  -p "${HOST_PORT}:19999" \
  -v "${VOL_CONFIG}:/etc/netdata" \
  -v "${VOL_LIB}:/var/lib/netdata" \
  -v "${VOL_CACHE}:/var/cache/netdata" \
  -v /:/host/root:ro,rslave \
  -v /etc/passwd:/host/etc/passwd:ro \
  -v /etc/group:/host/etc/group:ro \
  -v /etc/localtime:/etc/localtime:ro \
  -v /proc:/host/proc:ro \
  -v /sys:/host/sys:ro \
  -v /etc/os-release:/host/etc/os-release:ro \
  -v /var/log:/host/var/log:ro \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  -v /run/dbus:/run/dbus:ro \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  UI: http://127.0.0.1:${HOST_PORT}/v3
  image: ${IMG}
  gpu: $([ ${#GPU_ARGS[@]} -gt 0 ] && echo enabled || echo disabled)
  cloud: disabled (NETDATA_DISABLE_CLOUD=1)
EOF
