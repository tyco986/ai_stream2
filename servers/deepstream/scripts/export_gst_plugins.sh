#!/usr/bin/env bash
# Export DeepStream GStreamer plugin list from ${PROJECT_NAME}_deepstream into docs/.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
DS_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DOCS_DIR="${DS_ROOT}/docs"
UTILS_SCRIPT="${SCRIPT_DIR}/utils/export_gst_plugins.py"

CONTAINER="${DEEPSTREAM_CONTAINER:-${PROJECT_NAME}_deepstream}"
REMOTE_SCRIPT="/tmp/export_gst_plugins.py"
REMOTE_OUT="/tmp/deepstream_gst_plugins_out"

if ! docker inspect "${CONTAINER}" >/dev/null 2>&1; then
  echo "Container not found: ${CONTAINER}" >&2
  echo "Start it with: ${SCRIPT_DIR}/2_run_dev_container.sh" >&2
  exit 1
fi

mkdir -p "${DOCS_DIR}"

docker cp "${UTILS_SCRIPT}" "${CONTAINER}:${REMOTE_SCRIPT}"
docker exec "${CONTAINER}" rm -rf "${REMOTE_OUT}"
docker exec "${CONTAINER}" python3 "${REMOTE_SCRIPT}" --output-dir "${REMOTE_OUT}"
docker cp "${CONTAINER}:${REMOTE_OUT}/." "${DOCS_DIR}/"
docker exec "${CONTAINER}" rm -rf "${REMOTE_OUT}" "${REMOTE_SCRIPT}"

echo "Exported to ${DOCS_DIR}/deepstream_gst_plugins.csv and deepstream_gst_plugins.md"
