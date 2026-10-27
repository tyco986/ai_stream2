#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${BACKEND_HOST_PORT:-8001}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/puskas/ffmpeg/rtsp/probe"
URL="${1:-/root/media/upload/admin/video1.mp4}"
BODY="$(printf '{"urls":["%s"]}' "${URL}")"
USER_INFO="$(cd "$(dirname "$0")/.." && pwd)/user_info"
TOKEN="$(sed -n 's/.*"token":"\([^"]*\)".*/\1/p' "${USER_INFO}")"
[[ -n "${TOKEN}" ]]
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 --max-time 15 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "authorization: ${TOKEN}" \
  -H "Content-Type: application/json" \
  -H "Accept-Language: en" \
  -d "${BODY}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"authorization":"%s","Content-Type":"application/json","Accept-Language":"en"},"content":%s},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${TOKEN}" "${BODY}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
