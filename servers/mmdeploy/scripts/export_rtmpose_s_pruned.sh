#!/usr/bin/env bash
# Runs inside the mmdeploy image.
set -euo pipefail

MODEL_NAME="rtmpose-s-aic"
MMDEPLOY_ROOT="${MMDEPLOY_ROOT:-/root/workspace/mmdeploy}"
CKPT="/root/models/pt/${MODEL_NAME}.pth"
OUT_DIR="/root/models/onnx/${MODEL_NAME}"
WORK_DIR="/tmp/${MODEL_NAME}-mmdeploy"
DEMO_IMG="/tmp/${MODEL_NAME}-demo.jpg"
POSE_CFG="/opt/mmdeploy-src/configs/group_fisher_deploy_rtmpose-s_8xb256-420e_aic-coco-256x192.py"
DEPLOY_CFG="${MMDEPLOY_ROOT}/configs/mmpose/pose-detection_simcc_onnxruntime_dynamic.py"

[[ -f "${CKPT}" ]] || { echo "checkpoint not found: ${CKPT}" >&2; exit 1; }
[[ -f "${DEPLOY_CFG}" ]] || { echo "mmdeploy config not found: ${DEPLOY_CFG}" >&2; exit 1; }
[[ -f "${POSE_CFG}" ]] || { echo "pose config not found: ${POSE_CFG}" >&2; exit 1; }

python3 - "${DEMO_IMG}" <<'PY'
import sys

import numpy as np
import cv2

cv2.imwrite(sys.argv[1], np.zeros((256, 192, 3), dtype=np.uint8))
PY

rm -rf "${WORK_DIR}" "${OUT_DIR}"
mkdir -p "${WORK_DIR}" "${OUT_DIR}"

python3 "${MMDEPLOY_ROOT}/tools/deploy.py" \
  "${DEPLOY_CFG}" \
  "${POSE_CFG}" \
  "${CKPT}" \
  "${DEMO_IMG}" \
  --work-dir "${WORK_DIR}" \
  --device cpu

ONNX_SRC="${WORK_DIR}/end2end.onnx"
[[ -f "${ONNX_SRC}" ]] || { echo "mmdeploy did not produce ${ONNX_SRC}" >&2; exit 1; }
cp "${ONNX_SRC}" "${OUT_DIR}/${MODEL_NAME}.onnx"

cat > "${OUT_DIR}/labels.txt" <<'EOF'
nose
left_eye
right_eye
left_ear
right_ear
left_shoulder
right_shoulder
left_elbow
right_elbow
left_wrist
right_wrist
left_hip
right_hip
left_knee
right_knee
left_ankle
right_ankle
EOF

echo "exported ${OUT_DIR}/${MODEL_NAME}.onnx"
