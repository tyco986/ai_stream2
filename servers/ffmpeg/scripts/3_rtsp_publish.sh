#!/usr/bin/env bash
set -euo pipefail

HOST_PORT=8080
ENDPOINT="http://127.0.0.1:${HOST_PORT}/rtsp/publish"
if [[ $# -eq 0 ]]; then
  set -- video1 /root/attachments/videos/video1.mp4 video2 /root/attachments/videos/video2.mp4
fi
[[ $# -ge 2 && $(($# % 2)) -eq 0 ]]
PARTS=()
while [[ $# -gt 0 ]]; do
  NAME="${1//\\/\\\\}"
  NAME="${NAME//\"/\\\"}"
  PATH_VALUE="${2//\\/\\\\}"
  PATH_VALUE="${PATH_VALUE//\"/\\\"}"
  PARTS+=("\"${NAME}\":\"${PATH_VALUE}\"")
  shift 2
done
IFS=,
BODY="{\"paths\":{${PARTS[*]}}}"
unset IFS
RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 --max-time 60 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -H "Accept-Language: en" \
  -d "${BODY}")"
CONTENT="$(cat "${RESPONSE_BODY}")"
CONTENT="${CONTENT:-null}"
printf '{"request":{"method":"POST","url":"%s","headers":{"Content-Type":"application/json","Accept-Language":"en"},"content":%s},"response":{"code":%s,"content":%s}}\n' "${ENDPOINT}" "${BODY}" "${HTTP_CODE}" "${CONTENT}"
[[ "${HTTP_CODE}" == "200" ]]
