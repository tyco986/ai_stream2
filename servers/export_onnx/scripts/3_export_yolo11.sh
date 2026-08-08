#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

API_URL="http://127.0.0.1:8090"
ENDPOINT="${API_URL}/${PROJECT_NAME}/export_onnx/export_yolo11"

usage() {
  cat <<EOF
usage: $0 --input path/to/model.pt [--batch-size N] [--dynamic] [--conf F] [--iou F]

Export YOLO11 .pt to ONNX under models/onnx/{name}/ via ExportOnnx API.

Options:
  --input PATH         Weights (.pt) (required)
  --batch-size N       Static batch size (default 1)
  --dynamic            Enable dynamic batch axis
  --conf F             Confidence threshold (default 0.25)
  --iou F              IoU threshold (default 0.45)

Prerequisites: 1_build_image.sh, 2_run_container.sh

Note: YOLO26 weights must use 3_export_yolo26.sh.
EOF
}

INPUT=""
BATCH_SIZE=1
DYNAMIC=false
CONF=0.25
IOU=0.45

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="$2"
      shift 2
      ;;
    --batch-size)
      BATCH_SIZE="$2"
      shift 2
      ;;
    --dynamic)
      DYNAMIC=true
      shift
      ;;
    --conf)
      CONF="$2"
      shift 2
      ;;
    --iou)
      IOU="$2"
      shift 2
      ;;
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

[[ -n "$INPUT" ]] || { echo "--input is required" >&2; usage; exit 1; }

INPUT="$(realpath "$INPUT")"
[[ -f "$INPUT" ]] || { echo "weights not found: $INPUT" >&2; exit 1; }
[[ "${INPUT##*.}" == "pt" ]] || { echo "input must be a .pt file: $INPUT" >&2; exit 1; }

HTTP_CODE="$(curl -sS -w "%{http_code}" -o /tmp/export_onnx_export_response.json \
  -X POST "${ENDPOINT}" \
  -F "input=@${INPUT}" \
  -F "size=640" \
  -F "opset=18" \
  -F "batch=${BATCH_SIZE}" \
  -F "dynamic=${DYNAMIC}" \
  -F "conf=${CONF}" \
  -F "iou=${IOU}" \
  -F "simplify=false")"

cat /tmp/export_onnx_export_response.json
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
