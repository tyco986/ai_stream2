#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${UPDATOR_HOST_PORT:-8095}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/updator/pack"
BODY="${1:-}"

if [[ -z "${BODY}" ]]; then
  echo "usage: $0 '<json>'" >&2
  exit 1
fi

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  -H "Content-Type: application/json" \
  --data-binary "${BODY}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en","Content-Type":"application/json"},"content":%s},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${BODY}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
