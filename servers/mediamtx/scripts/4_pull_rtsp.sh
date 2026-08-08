#!/usr/bin/env bash
# Run from project root. MediaMTX pulls (proxies) an existing RTSP source.
set -euo pipefail

API="http://127.0.0.1:9997"
SOURCE=""
PATH_NAME=""

usage() {
  cat <<EOF
usage: $0 --source URL [options]

  --source URL        camera / upstream RTSP URL (required)
  --name NAME         MediaMTX path name (default: last path segment of --source)
  --api URL           Control API base (default: $API)

Mounts path with source=<URL> so clients can play:
  rtsp://127.0.0.1:8554/<name>
  http://127.0.0.1:8889/<name>/
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --source) SOURCE="$2"; shift 2 ;;
    --name) PATH_NAME="$2"; shift 2 ;;
    --api) API="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

if [[ -z "$SOURCE" ]]; then
  echo "error: --source is required" >&2
  usage >&2
  exit 1
fi

if [[ -z "$PATH_NAME" ]]; then
  PATH_NAME="${SOURCE##*/}"
  PATH_NAME="${PATH_NAME%%\?*}"
fi

if [[ -z "$PATH_NAME" || "$PATH_NAME" == "$SOURCE" ]]; then
  echo "error: could not derive path name; pass --name" >&2
  exit 1
fi

BODY=$(printf '{"source":"%s"}' "$SOURCE")
URL="${API}/v3/config/paths/add/${PATH_NAME}"

HTTP=$(curl -s -o /tmp/mediamtx_pull_body.$$ -w "%{http_code}" -X POST "$URL" \
  -H "Content-Type: application/json" \
  -d "$BODY")
BODY_OUT=$(cat /tmp/mediamtx_pull_body.$$)
rm -f /tmp/mediamtx_pull_body.$$

if [[ "$HTTP" == "200" ]]; then
  echo "$BODY_OUT"
  exit 0
fi

# Path already exists → replace (re-pull / update source).
URL="${API}/v3/config/paths/replace/${PATH_NAME}"
curl -s -X POST "$URL" \
  -H "Content-Type: application/json" \
  -d "$BODY"
echo
