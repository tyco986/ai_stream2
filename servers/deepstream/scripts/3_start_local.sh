#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
HOST_PORT="${DEEPSTREAM_HOST_PORT:-8094}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/start/local"
HOST_HISTORY="${ROOT}/history"
CONFIG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config) CONFIG="$2"; shift 2 ;;
    *) echo "usage: $0 --config path/to/config.zip" >&2; exit 1 ;;
  esac
done

[[ -n "${CONFIG}" ]]
CONFIG="$(realpath "${CONFIG}")"
[[ -f "${CONFIG}" ]]
[[ "${CONFIG}" == "${HOST_HISTORY}/"* ]]
CONFIG="/root/history/${CONFIG#"${HOST_HISTORY}/"}"

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  -H "Content-Type: application/json" \
  -d "{\"config\":\"${CONFIG}\"}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en","Content-Type":"application/json"},"content":{"config":"%s"}},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${CONFIG}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
