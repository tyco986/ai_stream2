#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
API_URL="http://127.0.0.1:8091"
ENDPOINT="${API_URL}/ai_stream2/generator/generate"
TEMPLATES_DIR="${ROOT}/servers/generator/templates"

usage() {
  cat <<EOF
usage: $0 --config PATH

Generate DeepStream pipeline configs via Generator API.

Options:
  --config PATH   Generator YAML (e.g. yolo26n_det_rtsp or servers/generator/templates/...)

Prerequisites: 1_build_image.sh, 2_run_container.sh
EOF
}

CONFIG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config)
      CONFIG="$2"
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

[[ -n "$CONFIG" ]] || { echo "--config is required" >&2; usage; exit 1; }

resolve_config_path() {
  local path="$1"
  if [[ -f "$path" ]]; then
    realpath "$path"
    return
  fi
  if [[ -f "${TEMPLATES_DIR}/${path}.yaml" ]]; then
    realpath "${TEMPLATES_DIR}/${path}.yaml"
    return
  fi
  if [[ -f "${TEMPLATES_DIR}/${path}" ]]; then
    realpath "${TEMPLATES_DIR}/${path}"
    return
  fi
  echo ""
}

CONFIG_PATH="$(resolve_config_path "$CONFIG")"
[[ -n "$CONFIG_PATH" ]] || { echo "config not found: $CONFIG" >&2; exit 1; }

HTTP_CODE="$(curl -sS -w "%{http_code}" -o /tmp/generator_response.json \
  -X POST "${ENDPOINT}" \
  -F "input=@${CONFIG_PATH}")"

cat /tmp/generator_response.json
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
