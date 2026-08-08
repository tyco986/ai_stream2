#!/usr/bin/env bash
# Run from project root. Stop MediaMTX pull (remove path config).
set -euo pipefail

API="http://127.0.0.1:9997"
PATH_NAME=""
RTSP=""

usage() {
  cat <<EOF
usage: $0 (--name NAME | --rtsp URL) [options]

  --name NAME         MediaMTX path name to remove
  --rtsp URL          derive path name from URL last segment (same as 4_pull_rtsp.sh)
  --api URL           Control API base (default: $API)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --name) PATH_NAME="$2"; shift 2 ;;
    --rtsp) RTSP="$2"; shift 2 ;;
    --api) API="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

if [[ -z "$PATH_NAME" && -n "$RTSP" ]]; then
  PATH_NAME="${RTSP##*/}"
  PATH_NAME="${PATH_NAME%%\?*}"
fi

if [[ -z "$PATH_NAME" ]]; then
  echo "error: pass --name or --rtsp" >&2
  usage >&2
  exit 1
fi

curl -s -X DELETE "${API}/v3/config/paths/delete/${PATH_NAME}"
echo
