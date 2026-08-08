#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8092"
ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/start_pipeline"
TEMPLATES_DIR="${ROOT}/servers/deepstream/templates"

usage() {
  cat <<EOF
usage: $0 --config PATH

Build and start a DeepStream pipeline via API from a template YAML.

Options:
  --config PATH   Pipeline template (e.g. yolo26n_det_image_pipeline or servers/deepstream/templates/...)

Prerequisites: 1_build_dev_image.sh or 1_build_prod_image.sh, 2_run_dev_container.sh or 2_run_prod_container.sh
Stop: docker stop ai_stream2_deepstream
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
  local candidate
  if [[ -f "$path" ]]; then
    realpath "$path"
    return
  fi
  for candidate in \
    "${TEMPLATES_DIR}/${path}.yml" \
    "${TEMPLATES_DIR}/${path}" \
    "${TEMPLATES_DIR}"/*/"${path}.yml" \
    "${TEMPLATES_DIR}"/*/"${path}"
  do
    if [[ -f "$candidate" ]]; then
      realpath "$candidate"
      return
    fi
  done
  echo ""
}

CONFIG_PATH="$(resolve_config_path "$CONFIG")"
[[ -n "$CONFIG_PATH" ]] || { echo "config not found: $CONFIG" >&2; exit 1; }

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -F "input=@${CONFIG_PATH}")"

cat "${RESPONSE_BODY}"
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
