#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/generator -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_generator_dev"
mkdir -p "${ROOT}/configs" "${ROOT}/logs" "${ROOT}/attachments"

# Docker Desktop 对 WSL /home bind 会挂空目录；必须用 \\wsl$\<distro>\ 反斜杠 UNC。
HOST_BIND="${ROOT}"
if [[ -n "${WSL_DISTRO_NAME:-}" ]]; then
  HOST_BIND="\\\\wsl\$\\${WSL_DISTRO_NAME}${ROOT//\//\\}"
fi

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_generator" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_generator" \
  --network "${PROJECT_NAME}_default" \
  -p 8091:8091 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${HOST_BIND}\\configs:/root/configs" \
  -v "${HOST_BIND}\\models:/root/models" \
  -v "${HOST_BIND}\\attachments:/root/attachments" \
  -v "${HOST_BIND}\\logs:/root/logs" \
  -v "${HOST_BIND}\\servers\\generator:/app" \
  "${IMAGE}"

echo "Generator API: http://127.0.0.1:8091/${PROJECT_NAME}/generator/generate"
echo "Swagger:       http://127.0.0.1:8091/docs"
echo "Mode:          dev image=${IMAGE}"
