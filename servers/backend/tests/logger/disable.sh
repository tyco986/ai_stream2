#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/backend/logs/disable"
USER_INFO="$(cd "$(dirname "$0")/.." && pwd)/user_info"
TOKEN="$(sed -n 's/.*"token":"\([^"]*\)".*/\1/p' "${USER_INFO}")"
[[ -n "${TOKEN}" ]]
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST \
  -H "authorization: ${TOKEN}" \
  -H "Accept-Language: en" \
  "${ENDPOINT}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"authorization":"%s","Accept-Language":"en"},"content":null},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${TOKEN}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
