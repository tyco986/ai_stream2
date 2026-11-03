#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${EXPORT_TRT_HOST_PORT:-8092}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/export_trt/logs"
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X DELETE \
  -H "Accept-Language: en" \
  "${ENDPOINT}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"DELETE","url":"%s","headers":{"Accept-Language":"en"},"content":null},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
