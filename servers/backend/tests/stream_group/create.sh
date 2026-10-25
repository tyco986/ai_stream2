#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
PARENT_GROUP_ID="00000000-0000-4000-8000-000000000001"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/backend/streams/groups/${PARENT_GROUP_ID}"
BODY='{"name":"probe"}'
USER_INFO="$(cd "$(dirname "$0")/.." && pwd)/user_info"
TOKEN="$(sed -n 's/.*"token":"\([^"]*\)".*/\1/p' "${USER_INFO}")"
[[ -n "${TOKEN}" ]]
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "authorization: ${TOKEN}" \
  -H "Content-Type: application/json" \
  -H "Accept-Language: en" \
  -d "${BODY}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"authorization":"%s","Content-Type":"application/json","Accept-Language":"en"},"content":%s},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${TOKEN}" "${BODY}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
GROUP_ID="$(sed -n 's/.*"id":"\([^"]*\)".*/\1/p' "${RESPONSE_BODY}")"
[[ -n "${GROUP_ID}" ]]
printf '%s\n' "${GROUP_ID}" > "$(cd "$(dirname "$0")" && pwd)/group_id"
