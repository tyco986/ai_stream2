#!/usr/bin/env bash
# Run from project root. Clear /root/logs/deepstream inside the deepstream container.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

CONTAINER="${DEEPSTREAM_CONTAINER:-${PROJECT_NAME}_deepstream}"
LOG_DIR="/root/logs/deepstream"

usage() {
  cat <<EOF
usage: $0

Clear all files under ${LOG_DIR} via docker exec on the deepstream container
(keeps the directory).

Environment:
  DEEPSTREAM_CONTAINER  Container name (default: \${PROJECT_NAME}_deepstream)

Prerequisites: deepstream container running
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if ! docker inspect "${CONTAINER}" >/dev/null 2>&1; then
  echo "Container not found: ${CONTAINER}" >&2
  echo "Start it with: servers/deepstream/scripts/2_run_dev_container.sh" >&2
  exit 1
fi

docker exec "${CONTAINER}" bash -c "mkdir -p '${LOG_DIR}' && find '${LOG_DIR}' -mindepth 1 -maxdepth 1 -exec rm -rf {} +"
echo "cleared: ${CONTAINER}:${LOG_DIR}"
