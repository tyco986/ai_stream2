#!/usr/bin/env bash
# Run from project root. Dev image + mount servers/generator -> /app.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

IMAGE="${PROJECT_NAME}_generator_dev"
mkdir -p "${ROOT}/configs" "${ROOT}/logs" "${ROOT}/attachments"

# Docker Desktop 对 WSL /home bind 会挂空目录，需 \\wsl$\<distro>\ UNC。
# WSL 内原生 docker-ce 必须用 Linux 路径，UNC 会被当成 volume 名而失败。
HOST_BIND="${ROOT}"
SEP="/"
if [[ -n "${WSL_DISTRO_NAME:-}" ]] && docker info --format '{{.OperatingSystem}}' 2>/dev/null | grep -qi 'Docker Desktop'; then
  HOST_BIND="\\\\wsl\$\\${WSL_DISTRO_NAME}${ROOT//\//\\}"
  SEP="\\"
fi

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_generator" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_generator" \
  --network "${PROJECT_NAME}_default" \
  -p 8091:8091 \
  -e PROJECT_NAME="${PROJECT_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${HOST_BIND}${SEP}configs:/root/configs" \
  -v "${HOST_BIND}${SEP}models:/root/models" \
  -v "${HOST_BIND}${SEP}attachments:/root/attachments" \
  -v "${HOST_BIND}${SEP}logs:/root/logs" \
  -v "${HOST_BIND}${SEP}servers${SEP}generator:/app" \
  "${IMAGE}"

echo "Generator API: http://127.0.0.1:8091/${PROJECT_NAME}/generator/generate"
echo "Swagger:       http://127.0.0.1:8091/docs"
echo "Mode:          dev image=${IMAGE}"
