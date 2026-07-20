#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

API_URL="http://127.0.0.1:8090"
ENDPOINT="${API_URL}/${PROJECT_NAME}/dsyolo/export_yolo11_seg_sahi"

usage() {
  cat <<EOF
usage: $0 --input path/to/model.pt [--batch-size N] [--dynamic]

Export YOLO11 seg .pt to SAHI ONNX under models/onnx/{name}-sahi/ via DsYolo API.

Options:
  --input PATH         Weights (.pt) (required)
  --batch-size N       Static batch size (default 1)
  --dynamic            Enable dynamic batch axis (recommended for SAHI)

Prerequisites: 1_build_image.sh, 2_run_container.sh
EOF
}

INPUT=""
BATCH_SIZE=1
DYNAMIC=false

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

HTTP_CODE="$(curl -sS -w "%{http_code}" -o /tmp/dsyolo_export_response.json \
  -X POST "${ENDPOINT}" \
  -F "input=@${INPUT}" \
  -F "size=640" \
  -F "opset=17" \
  -F "batch=${BATCH_SIZE}" \
  -F "dynamic=${DYNAMIC}" \
  -F "simplify=true")"

cat /tmp/dsyolo_export_response.json
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
