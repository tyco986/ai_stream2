#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8092"
ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/start_pipeline"
HEALTH_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/health"

usage() {
  cat <<EOF
usage: $0 --config PATH

Build and start a DeepStream pipeline via API from a YAML with type and config_dir.

Options:
  --config PATH   Start YAML (must exist; API reads type and config_dir)

Prerequisites: 1_build_dev_image.sh or 1_build_prod_image.sh, 2_run_dev_container.sh or 2_run_prod_container.sh
  (container runs deepstream_api from modules/api)
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
[[ -f "$CONFIG" ]] || { echo "config not found: $CONFIG" >&2; exit 1; }
CONFIG_PATH="$(realpath "$CONFIG")"

curl -sS --connect-timeout 2 "${HEALTH_ENDPOINT}" >/dev/null \
  || { echo "deepstream_api not ready: ${HEALTH_ENDPOINT}" >&2; exit 1; }

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -F "input=@${CONFIG_PATH}")"

cat "${RESPONSE_BODY}"
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
