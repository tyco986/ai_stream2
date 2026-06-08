#!/usr/bin/env bash
# Run from project root on the host. Requires ai_stream2_deepstream container.
set -euo pipefail

CONTAINER=ai_stream2_deepstream
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
INPUT=""
OUTPUT=""

usage() {
  cat <<EOF
usage: $0 --input PATH --output PATH

Build a TensorRT engine from ONNX inside ${CONTAINER} (trtexec).

Required:
  --input PATH    Host path to .onnx file (same directory must contain .onnx.data if used)
  --output PATH   Host path for .engine file (e.g. models/trt/foo.onnx_b1_gpu0_fp16.engine)

Copies the ONNX directory into the container, runs trtexec with --fp16, then copies
the engine back to --output.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --input)
      [[ $# -ge 2 ]] || { echo "--input requires a path" >&2; exit 1; }
      INPUT="$2"
      shift 2
      ;;
    --output)
      [[ $# -ge 2 ]] || { echo "--output requires a path" >&2; exit 1; }
      OUTPUT="$2"
      shift 2
      ;;
    *) echo "unknown argument: $1" >&2; usage >&2; exit 1 ;;
  esac
done

[[ -n "$INPUT" ]] || { echo "--input is required" >&2; usage >&2; exit 1; }
[[ -n "$OUTPUT" ]] || { echo "--output is required" >&2; usage >&2; exit 1; }

[[ "$INPUT" = /* ]] && INPUT_ABS="$INPUT" || INPUT_ABS="${ROOT}/${INPUT}"
[[ "$OUTPUT" = /* ]] && OUTPUT_ABS="$OUTPUT" || OUTPUT_ABS="${ROOT}/${OUTPUT}"
INPUT_DIR="$(dirname "$INPUT_ABS")"
OUTPUT_DIR="$(dirname "$OUTPUT_ABS")"

[[ -f "$INPUT_ABS" ]] || { echo "onnx not found: $INPUT_ABS" >&2; exit 1; }
[[ -d "$OUTPUT_DIR" ]] || { echo "output directory not found: $OUTPUT_DIR" >&2; exit 1; }
docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null | grep -q true \
  || { echo "container not running: $CONTAINER (run 1_run_container.sh first)" >&2; exit 1; }

ONNX_NAME="$(basename "$INPUT_ABS")"
ENGINE_NAME="$(basename "$OUTPUT_ABS")"
C="/tmp/export_engine"

docker exec "$CONTAINER" mkdir -p "$C"
docker cp "${INPUT_DIR}/." "${CONTAINER}:${C}/"

docker exec "$CONTAINER" trtexec \
  --onnx="${C}/${ONNX_NAME}" \
  --saveEngine="${C}/${ENGINE_NAME}" \
  --fp16 \
  --memPoolSize=workspace:4096

docker cp "${CONTAINER}:${C}/${ENGINE_NAME}" "$OUTPUT_ABS"
echo "engine written: $OUTPUT_ABS"
