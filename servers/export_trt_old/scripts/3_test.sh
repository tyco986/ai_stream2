#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

CONFIG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config)
      CONFIG="$2"
      shift 2
      ;;
    *)
      echo "usage: $0 --config path/to/config.yaml" >&2
      exit 1
      ;;
  esac
done

[[ -n "${CONFIG}" ]] || { echo "--config is required" >&2; exit 1; }

CONFIG="$(realpath "${CONFIG}")"
[[ -f "${CONFIG}" ]] || { echo "config not found: ${CONFIG}" >&2; exit 1; }

curl -sS -X POST "http://127.0.0.1:9000/${PROJECT_NAME}/export_trt/test" \
  -F "config=@${CONFIG}"
echo
