#!/usr/bin/env bash
# Export Ultralytics .pt to TensorRT .engine via ultralytics/ultralytics:latest-export.
set -euo pipefail

IMAGE="ultralytics/ultralytics:latest-export"
FORMAT="engine"
MODEL=""
EXTRA_YOLO_ARGS=()

usage() {
  cat <<EOF
Usage: $0 --model PATH [options]

Export a YOLO .pt weights file to TensorRT engine (yolo export) inside Docker.
Output is written next to the weights file on the host (same directory as --model).

Required:
  --model PATH          Host path to .pt weights

Options:
  --format NAME         Export format (default: engine)
  --help                Show this help

Additional yolo export options (passed through as key=value):
  --imgsz 640           --batch 1           --half
  --dynamic             --int8              --device 0
  --workspace 4         --simplify          --nms
  --data coco8.yaml     --end2end           ...

Examples:
  $0 --model ./yolo26n.pt
  $0 --model /data/yolo26n.pt --format engine --half --batch 1 --device 0
  $0 --model ./yolo26n.pt --imgsz 1280 --dynamic

Requires: Docker with NVIDIA runtime (--gpus all), image ${IMAGE}
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h | --help)
      usage
      exit 0
      ;;
    --model)
      [[ $# -ge 2 ]] || {
        echo "error: --model requires a path" >&2
        exit 1
      }
      MODEL="$2"
      shift 2
      ;;
    --format)
      [[ $# -ge 2 ]] || {
        echo "error: --format requires a value" >&2
        exit 1
      }
      FORMAT="$2"
      shift 2
      ;;
    --*)
      _key="${1#--}"
      shift
      if [[ $# -gt 0 && "$1" != --* ]]; then
        EXTRA_YOLO_ARGS+=("${_key}=${1}")
        shift
      else
        EXTRA_YOLO_ARGS+=("${_key}=True")
      fi
      ;;
    *)
      echo "error: unknown argument: $1 (use --help)" >&2
      exit 1
      ;;
  esac
done

[[ -n "${MODEL}" ]] || {
  usage >&2
  exit 1
}

[[ -f "${MODEL}" ]] || {
  echo "error: model file not found: ${MODEL}" >&2
  exit 1
}

MODEL_HOST="$(readlink -f "${MODEL}")"
MODEL_DIR="$(dirname "${MODEL_HOST}")"
MODEL_BASE="$(basename "${MODEL_HOST}")"
CONTAINER_MODEL="/w/${MODEL_BASE}"

DOCKER_TTY=()
if [[ -t 0 ]]; then
  DOCKER_TTY=(-it)
fi

YOLO_CMD=(
  yolo export
  "model=${CONTAINER_MODEL}"
  "format=${FORMAT}"
)
if [[ ${#EXTRA_YOLO_ARGS[@]} -gt 0 ]]; then
  YOLO_CMD+=("${EXTRA_YOLO_ARGS[@]}")
fi

echo "image:   ${IMAGE}"
echo "model:   ${MODEL_HOST}"
echo "format:  ${FORMAT}"
echo "command: ${YOLO_CMD[*]}"

exec docker run --rm "${DOCKER_TTY[@]}" --ipc=host --gpus all \
  -v "${MODEL_DIR}:/w" \
  -w /w \
  "${IMAGE}" \
  "${YOLO_CMD[@]}"
