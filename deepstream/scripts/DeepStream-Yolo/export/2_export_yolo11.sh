#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<EOF
usage: $0 -w path/to/model.pt [export options...]

Export YOLO11 .pt to ONNX + labels.txt for DeepStream (container: ultralytics_export).

Required:
  -w PATH              Host path to weights (.pt)

Optional (passed to export_yolo11.py):
  -s, --size H [W]     Inference size (default 640)
  --opset N            ONNX opset (default 18; 17 may fail on newer PyTorch)
  --simplify           Run ONNX simplifier
  --dynamic            Dynamic batch axis
  --batch N            Static batch size (default 1; incompatible with --dynamic)

Output:
  deepstream/models/onnx/<model_stem>/{model_stem}.onnx
  deepstream/models/onnx/<model_stem>/{model_stem}.onnx.data
  deepstream/models/onnx/<model_stem>/labels.txt

Prerequisites: 0_pull_base_image.sh, 1_run_container.sh

Note: YOLO26 weights must use 2_export_yolo26.sh (export_yolo26.py).
EOF
}

case "${1:-}" in -h|--help|help) usage; exit 0 ;; esac

[[ $# -ge 2 && "$1" == -w ]] || { usage >&2; exit 1; }

W=$2
shift 2
F=$(basename "$W")
N="${F%.*}"
DS="$(cd "$(dirname "$0")/../../.." && pwd)"
OUT="${DS}/models/onnx/${N}"
C="/tmp/export/${N}"

mkdir -p "$OUT"
docker exec ultralytics_export mkdir -p "$C"
docker cp "$W" "ultralytics_export:${C}/${F}"
docker exec -w "$C" ultralytics_export python3 /app/DeepStream-Yolo-master/utils/export_yolo11.py -w "${C}/${F}" --opset 18 "$@"
docker cp "ultralytics_export:${C}/${N}.onnx" "${OUT}/${N}.onnx"
docker cp "ultralytics_export:${C}/${N}.onnx.data" "${OUT}/${N}.onnx.data"
docker cp "ultralytics_export:${C}/labels.txt" "${OUT}/labels.txt"
