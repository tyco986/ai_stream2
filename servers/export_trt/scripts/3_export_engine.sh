#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:9000"
ENDPOINT="${API_URL}/${PROJECT_NAME}/export_trt/export_engine"

usage() {
  cat <<EOF
usage: $0 --input path/to/onnx_folder [--batch-size N] [--precision fp16|fp32|int8] [--builder-optimization-level N]

Export TensorRT engine under models/trt/{name}/ via Export TRT API.

Options:
  --input PATH         ONNX folder under models/ (required)
  --batch-size N       Batch size for dynamic ONNX (default: omit)
  --precision VALUE    Engine precision: fp32, fp16, int8 (default: fp16)
  --builder-optimization-level N  trtexec builderOptimizationLevel (default: omit)

Prerequisites: 1_build_image.sh, 2_run_container.sh
EOF
}

INPUT=""
BATCH_SIZE=""
PRECISION="fp16"
BUILDER_OPTIMIZATION_LEVEL=""

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
    --precision)
      PRECISION="$2"
      shift 2
      ;;
    --builder-optimization-level)
      BUILDER_OPTIMIZATION_LEVEL="$2"
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
[[ -d "$INPUT" ]] || { echo "input not found: $INPUT" >&2; exit 1; }

REL="${INPUT#${ROOT}/models/}"
if [[ "$REL" == "$INPUT" ]]; then
  echo "input must be under ${ROOT}/models/" >&2
  exit 1
fi
CONTAINER_INPUT="/root/models/${REL}"

HTTP_CODE="$(curl -sS -w "%{http_code}" -o /tmp/export_trt_response.json \
  -X POST "${ENDPOINT}" \
  -F "input=${CONTAINER_INPUT}" \
  -F "precision=${PRECISION}" \
  ${BATCH_SIZE:+-F "batch_size=${BATCH_SIZE}"} \
  ${BUILDER_OPTIMIZATION_LEVEL:+-F "builder_optimization_level=${BUILDER_OPTIMIZATION_LEVEL}"})"

cat /tmp/export_trt_response.json
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
