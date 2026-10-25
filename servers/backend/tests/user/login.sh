#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/backend/users/login"
BODY='{"username":"admin","passwd":"admin"}'
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -H "Accept-Language: en" \
  -d "${BODY}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"Content-Type":"application/json","Accept-Language":"en"},"content":%s},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${BODY}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
TOKEN="$(sed -n 's/.*"token":"\([^"]*\)".*/\1/p' "${RESPONSE_BODY}")"
USER_ID="$(sed -n 's/.*"id":"\([^"]*\)".*/\1/p' "${RESPONSE_BODY}")"
USERNAME="$(sed -n 's/.*"username":"\([^"]*\)".*/\1/p' "${RESPONSE_BODY}")"
[[ -n "${TOKEN}" && -n "${USER_ID}" && -n "${USERNAME}" ]]
TESTS_DIR="$(cd "$(dirname "$0")/.." && pwd)"
printf '{"token":"%s","id":"%s","username":"%s"}\n' "${TOKEN}" "${USER_ID}" "${USERNAME}" > "${TESTS_DIR}/user_info"
