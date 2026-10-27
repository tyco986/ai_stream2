#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${EXPORT_TRT_HOST_PORT:-8080}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/export_trt/export"
INPUT=""
CONFIG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input) INPUT="$2"; shift 2 ;;
    --config) CONFIG="$2"; shift 2 ;;
    *) echo "usage: $0 --input path/to/model.onnx --config path/to/config.yaml" >&2; exit 1 ;;
  esac
done

[[ -n "${INPUT}" && -n "${CONFIG}" ]]
INPUT="$(realpath "${INPUT}")"
CONFIG="$(realpath "${CONFIG}")"
[[ -f "${INPUT}" && -f "${CONFIG}" ]]

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  -F "input=@${INPUT}" \
  -F "config=@${CONFIG}")"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en"},"content":{"input":"%s","config":"%s"}},"response":{"code":%s,"content":"multipart(zip)"}}\n' "${ENDPOINT}" "${INPUT}" "${CONFIG}" "${HTTP_CODE}"
[[ "${HTTP_CODE}" == "200" ]]
