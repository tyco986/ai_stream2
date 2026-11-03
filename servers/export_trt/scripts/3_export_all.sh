#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
EXPORT="$(cd "$(dirname "$0")" && pwd)/3_export.sh"
HISTORY="${ROOT}/history/export_onnx"
TEMPLATES="${ROOT}/servers/export_trt/templates"

"${EXPORT}" --input "${HISTORY}/yolo10x-fire-smoke.zip" --config "${TEMPLATES}/yolo10x_smoke_fire.yaml"
"${EXPORT}" --input "${HISTORY}/yolo11n.zip" --config "${TEMPLATES}/yolo11_det.yaml"
"${EXPORT}" --input "${HISTORY}/yolo11n-seg.zip" --config "${TEMPLATES}/yolo11_seg.yaml"
"${EXPORT}" --input "${HISTORY}/yolo26n.zip" --config "${TEMPLATES}/yolo26_det.yaml"
"${EXPORT}" --input "${HISTORY}/yolo26n-pose.zip" --config "${TEMPLATES}/yolo26_pose.yaml"
"${EXPORT}" --input "${HISTORY}/yolo26n-seg.zip" --config "${TEMPLATES}/yolo26_seg.yaml"
"${EXPORT}" --input "${HISTORY}/stgcnpp-ntu60-j.zip" --config "${TEMPLATES}/stgcnpp.yaml"
"${EXPORT}" --input "${HISTORY}/rtmpose-s-body8.zip" --config "${TEMPLATES}/rtmpose-s-body8.yaml"
"${EXPORT}" --input "${HISTORY}/rtmpose-t-body8.zip" --config "${TEMPLATES}/rtmpose-t-body8.yaml"
