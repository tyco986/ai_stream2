#!/usr/bin/env bash
# Run on host: sync demo configs into container and start action recognition sample.
#
# Usage: ./run_demo.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTAINER="${DEEPSTREAM_CONTAINER_NAME:-ai_stream2_deepstream}"
RUN_DIR="/tmp/actionrecognition-demo"
SAMPLE_LABELS="/opt/nvidia/deepstream/deepstream-9.0/sources/apps/sample_apps/deepstream-3d-action-recognition/labels.txt"

command -v docker >/dev/null || { echo "ERROR: docker required" >&2; exit 1; }
docker ps --format '{{.Names}}' | grep -qx "${CONTAINER}" || {
  echo "ERROR: container ${CONTAINER} is not running" >&2
  exit 1
}

if [[ ! -f "${SCRIPT_DIR}/labels.txt" ]]; then
  echo "=> Copy labels.txt from sample app"
  docker cp "${CONTAINER}:${SAMPLE_LABELS}" "${SCRIPT_DIR}/labels.txt"
fi

echo "=> Sync ${SCRIPT_DIR} -> ${CONTAINER}:${RUN_DIR}"
docker exec "${CONTAINER}" rm -rf "${RUN_DIR}"
docker exec "${CONTAINER}" mkdir -p "${RUN_DIR}"
docker cp "${SCRIPT_DIR}/." "${CONTAINER}:${RUN_DIR}/"

echo "=> deepstream-3d-action-recognition (config: demo/deepstream_action_recognition_config.txt)"
exec docker exec -it -w "${RUN_DIR}" "${CONTAINER}" \
  deepstream-3d-action-recognition -c deepstream_action_recognition_config.txt
