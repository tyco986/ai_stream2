#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
EXPORT="$(cd "$(dirname "$0")" && pwd)/3_export.sh"
SOURCE="${ROOT}/models/source"
TEMPLATES="${ROOT}/servers/export_onnx/templates"

"${EXPORT}" --input "${SOURCE}/yolo10x-fire-smoke.pt" --config "${TEMPLATES}/yolo10_det.yaml"
"${EXPORT}" --input "${SOURCE}/yolo11n.pt" --config "${TEMPLATES}/yolo11_det.yaml"
"${EXPORT}" --input "${SOURCE}/yolo11n-seg.pt" --config "${TEMPLATES}/yolo11_seg.yaml"
"${EXPORT}" --input "${SOURCE}/yolo26n.pt" --config "${TEMPLATES}/yolo26_det.yaml"
"${EXPORT}" --input "${SOURCE}/yolo26n-pose.pt" --config "${TEMPLATES}/yolo26_pose.yaml"
"${EXPORT}" --input "${SOURCE}/yolo26n-seg.pt" --config "${TEMPLATES}/yolo26_seg.yaml"
"${EXPORT}" --input "${SOURCE}/stgcnpp-ntu60-j.pth" --config "${TEMPLATES}/stgcnpp.yaml"
"${EXPORT}" --input "${SOURCE}/rtmpose-s-body8.onnx" --config "${TEMPLATES}/rtmpose-s-body8.yaml"
"${EXPORT}" --input "${SOURCE}/rtmpose-t-body8.onnx" --config "${TEMPLATES}/rtmpose-t-body8.yaml"
