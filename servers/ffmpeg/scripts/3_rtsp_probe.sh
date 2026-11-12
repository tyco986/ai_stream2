#!/usr/bin/env bash
set -euo pipefail

HOST_PORT=8080
ENDPOINT="http://127.0.0.1:${HOST_PORT}/rtsp/probe"
[[ $# -gt 0 ]]
PARTS=()
for url in "$@"; do
  PARTS+=("\"${url//\"/\\\"}\"")
done
IFS=,
BODY="{\"urls\":[${PARTS[*]}]}"
unset IFS
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 --max-time 30 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -H "Accept-Language: en" \
  -d "${BODY}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"Content-Type":"application/json","Accept-Language":"en"},"content":%s},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${BODY}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
