#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${UPDATOR_HOST_PORT:-8095}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/updator/update"
FILE_PATH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --file-path) FILE_PATH="$2"; shift 2 ;;
    *) echo "usage: $0 [--file-path /root/updator/update.zip]" >&2; exit 1 ;;
  esac
done

BODY="{}"
if [[ -n "${FILE_PATH}" ]]; then
  BODY="$(printf '{"file_path":"%s"}' "${FILE_PATH}")"
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
