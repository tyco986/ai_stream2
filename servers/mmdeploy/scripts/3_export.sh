#!/usr/bin/env bash
# Run from project root. Export RTMPose-s-aic-coco-pruned ONNX via MMDeploy.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

MODEL_NAME="rtmpose-s-aic"
NAME="${PROJECT_NAME}_mmdeploy"
CKPT_HOST="${ROOT}/models/pt/${MODEL_NAME}.pth"

mkdir -p "${ROOT}/models/pt" "${ROOT}/models/onnx" "${ROOT}/logs"

docker inspect -f '{{.State.Running}}' "${NAME}" 2>/dev/null | grep -qx true || {
  echo "container not running: ${NAME}; run servers/mmdeploy/scripts/1_run_container.sh" >&2
  exit 1
}

[[ -f "${CKPT_HOST}" ]] || { echo "checkpoint not found: ${CKPT_HOST}" >&2; exit 1; }

docker exec "${NAME}" bash /opt/mmdeploy-src/scripts/export_rtmpose_s_pruned.sh
