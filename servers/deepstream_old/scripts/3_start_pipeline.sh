#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8092"
ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/start_pipeline"
HEALTH_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/health"
HOST_CONFIGS="${ROOT}/configs"
CONTAINER_CONFIGS="/root/configs"

usage() {
  cat <<EOF
usage: $0 --config DIR

Start a DeepStream pipeline via API. DIR is a generator config directory
(host path under configs/ or container path /root/configs/...).

Options:
  --config DIR   Config directory (must exist; API reads params.yml type)

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
[[ -d "$CONFIG" ]] || { echo "config dir not found: $CONFIG" >&2; exit 1; }
CONFIG_PATH="$(realpath "$CONFIG")"
CONTAINER_DIR="$CONFIG_PATH"
if [[ "$CONFIG_PATH" == "${HOST_CONFIGS}/"* ]]; then
  CONTAINER_DIR="${CONTAINER_CONFIGS}/${CONFIG_PATH#"${HOST_CONFIGS}/"}"
fi

curl -sS --connect-timeout 2 "${HEALTH_ENDPOINT}" >/dev/null \
  || { echo "deepstream_api not ready: ${HEALTH_ENDPOINT}" >&2; exit 1; }

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -d "{\"config_dir\":\"${CONTAINER_DIR}\"}")"

cat "${RESPONSE_BODY}"
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
