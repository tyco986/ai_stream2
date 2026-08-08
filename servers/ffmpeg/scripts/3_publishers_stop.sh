#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

API_URL="http://127.0.0.1:8080"
ENDPOINT="${API_URL}/${PROJECT_NAME}/ffmpeg/rtsp/publishers"

usage() {
  cat <<EOF
usage: $0 [--name NAME]

Stop RTSP publisher(s).
  no --name     DELETE /rtsp/publishers (stop all)
  --name NAME   DELETE /rtsp/publishers/{name}

Options:
  --name NAME          Publisher name (optional; omit to stop all)

Prerequisites: 1_build_dev_image.sh, 2_run_dev_container.sh
EOF
}

NAME=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --name)
      NAME="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

if [[ -z "${NAME}" ]]; then
  HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" -X DELETE "${ENDPOINT}")"
else
  ENCODED="$(
    python3 -c "import urllib.parse,sys; print(urllib.parse.quote(sys.argv[1], safe=''))" "${NAME}"
  )"
  HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" -X DELETE "${ENDPOINT}/${ENCODED}")"
fi

if [[ "${HTTP_CODE}" == "200" ]]; then
  cat "${RESPONSE_BODY}"
else
  cat "${RESPONSE_BODY}"
  exit 1
fi
