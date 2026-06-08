#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
DEFAULT_WEIGHT="${ROOT}/models/pt/yolo11n-seg.pt"

usage() {
  cat <<EOF
usage: $0 [-w path/to/model.pt] [export options...]

Export YOLO11-Seg .pt to ONNX + labels.txt for DeepStream (container: DeepStream-Yolo).

Optional:
  -w PATH              Host path to weights (.pt); default: models/pt/yolo11n-seg.pt

Other flags (passed to export_yolo11_seg.py):
  -s, --size H [W]     Inference size (default 640)
  --opset N            ONNX opset (default 18; 17 may fail on newer PyTorch)
  --simplify           Run ONNX simplifier
  --dynamic            Dynamic batch axis
  --batch N            Static batch size (default 1; incompatible with --dynamic)
  --conf-threshold F   Detection confidence threshold (default 0.25)
  --iou-threshold F    NMS IoU threshold (default 0.45)
  --max-detections N   Max detections (default 100)

Output (under project root):
  models/trt/<model_stem>/{model_stem}.onnx
  models/trt/<model_stem>/{model_stem}.onnx.data
  models/trt/<model_stem>/labels.txt

Prerequisites: 0_pull_base_image.sh, 1_run_container.sh
EOF
}

case "${1:-}" in -h|--help|help) usage; exit 0 ;; esac

if [[ $# -ge 2 && "$1" == -w ]]; then
  W=$2
  shift 2
else
  W="${DEFAULT_WEIGHT}"
fi

[[ -f "$W" ]] || { echo "weights not found: $W" >&2; exit 1; }

F=$(basename "$W")
N="${F%.*}"
OUT="${ROOT}/models/trt/${N}"
C="/tmp/export/${N}"

mkdir -p "$OUT"
docker exec DeepStream-Yolo mkdir -p "$C"
docker cp "$W" "DeepStream-Yolo:${C}/${F}"
docker exec -w "$C" DeepStream-Yolo python3 /app/DeepStream-Yolo-Seg-master/utils/export_yolo11_seg.py -w "${C}/${F}" --opset 18 "$@"
docker cp "DeepStream-Yolo:${C}/${N}.onnx" "${OUT}/${N}.onnx"
docker cp "DeepStream-Yolo:${C}/${N}.onnx.data" "${OUT}/${N}.onnx.data"
docker cp "DeepStream-Yolo:${C}/labels.txt" "${OUT}/labels.txt"
