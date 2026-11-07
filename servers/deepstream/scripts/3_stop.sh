#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${DEEPSTREAM_HOST_PORT:-8094}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/stop"
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  -H "Content-Type: application/json" \
  -d '{}')"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en","Content-Type":"application/json"},"content":{}},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
