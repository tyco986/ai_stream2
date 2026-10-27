#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${EXPORT_ONNX_HOST_PORT:-8090}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/export_onnx/history"
PATH_ARG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --filename) PATH_ARG="$2"; shift 2 ;;
    *) echo "usage: $0 [--filename file.zip]" >&2; exit 1 ;;
  esac
done

ARGS=()
PATH_JSON="null"
if [[ -n "${PATH_ARG}" ]]; then
  ARGS+=(--data-urlencode "filename=${PATH_ARG}")
  PATH_JSON="\"${PATH_ARG}\""
fi

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -G -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  "${ARGS[@]+"${ARGS[@]}"}")"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en"},"content":{"filename":%s}},"response":{"code":%s,"content":"filename:multipart(zip)"}}\n' "${ENDPOINT}" "${PATH_JSON}" "${HTTP_CODE}"
[[ "${HTTP_CODE}" == "200" ]]
