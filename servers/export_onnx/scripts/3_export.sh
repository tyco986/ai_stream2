#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

INPUT=""
CONFIG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="$2"
      shift 2
      ;;
    --config)
      CONFIG="$2"
      shift 2
      ;;
    *)
      echo "usage: $0 --input path/to/model.pt --config path/to/config.yaml" >&2
      exit 1
      ;;
  esac
done

[[ -n "${INPUT}" ]] || { echo "--input is required" >&2; exit 1; }
[[ -n "${CONFIG}" ]] || { echo "--config is required" >&2; exit 1; }

INPUT="$(realpath "${INPUT}")"
CONFIG="$(realpath "${CONFIG}")"
[[ -f "${INPUT}" ]] || { echo "weights not found: ${INPUT}" >&2; exit 1; }
[[ -f "${CONFIG}" ]] || { echo "config not found: ${CONFIG}" >&2; exit 1; }

curl -sS -X POST "http://127.0.0.1:8090/${PROJECT_NAME}/export_onnx/export" \
  -F "input=@${INPUT}" \
  -F "config=@${CONFIG}"
echo
