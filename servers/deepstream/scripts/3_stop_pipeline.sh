#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

API_URL="http://127.0.0.1:8092"
ENDPOINT="${API_URL}/ai_stream2/deepstream/stop_pipeline"

usage() {
  cat <<EOF
usage: $0 --name NAME

Stop a running pipeline via DeepStream API.

Options:
  --name NAME    Pipeline instance name (required)

Prerequisites: 3_start_pipeline.sh
EOF
}

NAME=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --name)
      NAME="$2"
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

[[ -n "$NAME" ]] || { echo "--name is required" >&2; usage; exit 1; }

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -d "{\"name\":\"${NAME}\"}")"

cat "${RESPONSE_BODY}"
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
