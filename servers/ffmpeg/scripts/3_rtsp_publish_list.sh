#!/usr/bin/env bash
set -euo pipefail

HOST_PORT=8080
ENDPOINT="http://127.0.0.1:${HOST_PORT}/rtsp/publish"
QUERY=()
for name in "$@"; do
  QUERY+=(--data-urlencode "names=${name}")
done
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -G --connect-timeout 2 --max-time 30 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  "${QUERY[@]}" \
  -H "Accept-Language: en" \
  "${ENDPOINT}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"GET","url":"%s","headers":{"Accept-Language":"en"},"content":null},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
