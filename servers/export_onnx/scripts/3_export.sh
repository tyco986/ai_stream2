#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${EXPORT_ONNX_HOST_PORT:-8090}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/export_onnx/export"
INPUT=""
CONFIG=""
LABEL=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input) INPUT="$2"; shift 2 ;;
    --config) CONFIG="$2"; shift 2 ;;
    --label) LABEL="$2"; shift 2 ;;
    *) echo "usage: $0 --input path/to/model.pt --config path/to/config.yaml [--label path/to/labels.txt]" >&2; exit 1 ;;
  esac
done

[[ -n "${INPUT}" && -n "${CONFIG}" ]]
INPUT="$(realpath "${INPUT}")"
CONFIG="$(realpath "${CONFIG}")"
[[ -f "${INPUT}" && -f "${CONFIG}" ]]

ARGS=(-F "input=@${INPUT}" -F "config=@${CONFIG}")
LABEL_JSON="null"
if [[ -n "${LABEL}" ]]; then
  LABEL="$(realpath "${LABEL}")"
  [[ -f "${LABEL}" ]]
  ARGS+=(-F "label=@${LABEL}")
  LABEL_JSON="\"${LABEL}\""
fi

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  "${ARGS[@]}")"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en"},"content":{"input":"%s","config":"%s","label":%s}},"response":{"code":%s,"content":"multipart(zip)"}}\n' "${ENDPOINT}" "${INPUT}" "${CONFIG}" "${LABEL_JSON}" "${HTTP_CODE}"
[[ "${HTTP_CODE}" == "200" ]]
