#!/usr/bin/env bash
# Kafka command smoke: stop_rolling / start_rolling, switch_preview, toggle_osd,
# and orphan stop_recording -> clip_failed (verified via container logs).
#
# Does NOT wait for rolling MP4s or clip extraction (see kafka_storage_e2e.sh).
#
# Prerequisites: same as kafka_storage_e2e.sh — kafka + deepstream up, stream registered.
#
# Usage:
#   ./deepstream/script/kafka_commands_misc_e2e.sh
#   CAMERA_ID=demo_cam1 ./deepstream/script/kafka_commands_misc_e2e.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${PROJECT_ROOT}"

CAMERA_ID="${CAMERA_ID:-demo_cam1}"
KAFKA_TOPIC="${KAFKA_TOPIC:-deepstream-commands}"

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing command: $1" >&2
    exit 1
  }
}

require_cmd docker
require_cmd python3
require_cmd curl

kafka_send_json() {
  local payload="$1"
  printf '%s\n' "${payload}" | docker compose exec -T kafka rpk topic produce "${KAFKA_TOPIC}"
}

ds_ready() {
  curl -sS --connect-timeout 3 "http://127.0.0.1:9000/api/v1/health/get-dsready-state" \
    | grep -q '"ds-ready"[[:space:]]*:[[:space:]]*"YES"'
}

camera_in_pipeline() {
  curl -sS --connect-timeout 3 "http://127.0.0.1:9000/api/v1/stream/get-stream-info" | grep -q "\"camera_id\"[[:space:]]*:[[:space:]]*\"${CAMERA_ID}\""
}

resolve_numeric_source_id() {
  python3 <<PY
import json
import urllib.request

cam = "${CAMERA_ID}"
with urllib.request.urlopen("http://127.0.0.1:9000/api/v1/stream/get-stream-info", timeout=10) as r:
    data = json.load(r)

def walk(o):
    if isinstance(o, dict):
        for k, v in o.items():
            if k == "camera_id" and str(v) == cam:
                return o
            got = walk(v)
            if got is not None:
                return got
    elif isinstance(o, list):
        for item in o:
            got = walk(item)
            if got is not None:
                return got
    return None

row = walk(data)
if not row:
    raise SystemExit("camera_id not found in stream-info")
for key in ("source_id", "sourceId", "source-index"):
    if key in row:
        v = row[key]
        print(int(v) if not isinstance(v, int) else v)
        raise SystemExit(0)
raise SystemExit("source_id not found for camera")
PY
}

if ! ds_ready; then
  echo "DeepStream is not ds-ready." >&2
  exit 1
fi

if ! camera_in_pipeline; then
  echo "camera_id=${CAMERA_ID} not in stream-info. Add stream first." >&2
  exit 1
fi

SOURCE_NUM="$(resolve_numeric_source_id)"
echo "=> Resolved ${CAMERA_ID} -> source_id (int)=${SOURCE_NUM}"

echo "=> [1/5] Kafka: stop_rolling then start_rolling"
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'stop_rolling','source_id':'${CAMERA_ID}'}))")"
sleep 1
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'start_rolling','source_id':'${CAMERA_ID}'}))")"

echo "=> [2/5] Kafka: switch_preview (single source ${SOURCE_NUM}, then mosaic -1)"
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'switch_preview','source_id':${SOURCE_NUM}}))")"
sleep 1
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'switch_preview','source_id':-1}))")"

echo "=> [3/5] Kafka: toggle_osd off / on"
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'toggle_osd','show':False}))")"
sleep 1
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'toggle_osd','show':True}))")"

echo "=> [4/5] Kafka: stop_recording without start_recording (expect clip_failed)"
REQ="$(python3 -c "import uuid; print(str(uuid.uuid4()))")"
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'stop_recording','source_id':'${CAMERA_ID}','request_id':'${REQ}','end_ts':'2026-01-15T12:00:00Z'}))")"

echo "=> [5/5] Verify DeepStream published clip_failed for request_id=${REQ}"
sleep 2
if docker compose logs deepstream --since 2m 2>/dev/null | grep -F "Published clip_failed request_id=${REQ}"; then
  echo "clip_failed log line OK"
else
  echo "WARN: Did not find Published clip_failed log for this request_id. Recent logs:" >&2
  docker compose logs deepstream --tail 40 2>/dev/null || true
  exit 1
fi

echo
echo "=== kafka_commands_misc_e2e: all checks passed ==="
