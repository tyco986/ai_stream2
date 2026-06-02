#!/usr/bin/env bash
set -euo pipefail

RTSP="rtsp://127.0.0.1:8554/video1"
ROOT="/recordings"
DURATION="1h"
DEL_AFTER="24h"

usage() {
  cat <<EOF
usage: $0 [options]

  --rtsp URL          RTSP URL (default: $RTSP)
  --root DIR          recording root in container (default: $ROOT)
  --duration DUR      recordSegmentDuration (default: $DURATION)
  --del-after DUR     recordDeleteAfter (default: $DEL_AFTER)

duration / del-after: Go duration, e.g. 300ms, 30s, 5m, 1h, 2h30m
  units: ns, us (µs), ms, s, m, h. Use 0s for del-after to disable deletion.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rtsp) RTSP="$2"; shift 2 ;;
    --root) ROOT="$2"; shift 2 ;;
    --duration) DURATION="$2"; shift 2 ;;
    --del-after) DEL_AFTER="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

ROOT="${ROOT%/}"
PATH_NAME="${RTSP##*/}"; PATH_NAME="${PATH_NAME%%\?*}"
RECORD_PATH="${ROOT}/%path/%Y-%m-%d_%H-%M-%S-%f"

BODY=$(printf '{"record":true,"recordPath":"%s","recordSegmentDuration":"%s","recordDeleteAfter":"%s"}' \
  "$RECORD_PATH" "$DURATION" "$DEL_AFTER")

curl -s -X POST "http://127.0.0.1:9997/v3/config/paths/replace/${PATH_NAME}" \
  -H "Content-Type: application/json" \
  -d "$BODY"
