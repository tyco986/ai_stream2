#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/health"
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -H "Accept-Language: en" \
  "${ENDPOINT}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"GET","url":"%s","headers":{"Accept-Language":"en"},"content":null},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
