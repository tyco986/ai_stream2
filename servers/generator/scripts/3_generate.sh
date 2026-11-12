#!/usr/bin/env bash
set -euo pipefail

HOST_PORT="${GENERATOR_HOST_PORT:-8091}"
ENDPOINT="http://127.0.0.1:${HOST_PORT}/ai_stream2/generator/generate"
YAML=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --yaml) YAML="$2"; shift 2 ;;
    *) echo "usage: $0 --yaml path/to/config.yaml" >&2; exit 1 ;;
  esac
done

[[ -n "${YAML}" ]]
YAML="$(realpath "${YAML}")"
[[ -f "${YAML}" ]]

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS --connect-timeout 2 -o "${RESPONSE_BODY}" -w "%{http_code}" \
  -X POST "${ENDPOINT}" \
  -H "Accept-Language: en" \
  -F "input=@${YAML}")"
printf '{"request":{"method":"POST","url":"%s","headers":{"Accept-Language":"en"},"content":{"yaml":"%s"}},"response":{"code":%s,"content":"multipart(zip)"}}\n' "${ENDPOINT}" "${YAML}" "${HTTP_CODE}"
[[ "${HTTP_CODE}" == "200" ]]
