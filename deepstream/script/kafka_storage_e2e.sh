#!/usr/bin/env bash
# Kafka command smoke test: rolling record, screenshot, manual clip (single segment),
# and manual clip spanning two rolling segments (ffmpeg concat).
#
# Prerequisites:
#   - Project root: docker compose with kafka + deepstream up, streams added (e.g. start_local_demo.sh).
#   - Host: docker, python3, ffprobe (for wall-clock math on mounted MP4s).
#   - Default test tuning: DS_RECORDING_SEGMENT_SEC=30 (30s per rolling segment; ~90s wall clock
#     yields ~3 segments). start_local_demo.sh sets 30 unless you override.
#     If deepstream was started without this, set e.g. DS_RECORDING_SEGMENT_SEC=30 docker compose up -d deepstream
#
# Usage:
#   ./deepstream/script/kafka_storage_e2e.sh
#   CAMERA_ID=demo_cam1 ./deepstream/script/kafka_storage_e2e.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${PROJECT_ROOT}"

CAMERA_ID="${CAMERA_ID:-demo_cam1}"
STORAGE="${STORAGE:-${PROJECT_ROOT}/deepstream/storage}"
KAFKA_TOPIC="${KAFKA_TOPIC:-deepstream-commands}"

# Tuned for DS_RECORDING_SEGMENT_SEC=30 + buffer archive min age (~45s): second rolling file often >90s after the first.
WAIT_FIRST_SEGMENT_SEC="${WAIT_FIRST_SEGMENT_SEC:-90}"
WAIT_SECOND_SEGMENT_SEC="${WAIT_SECOND_SEGMENT_SEC:-150}"
# Async clip extraction
WAIT_CLIP_SEC="${WAIT_CLIP_SEC:-180}"

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing command: $1" >&2
    exit 1
  }
}

require_cmd docker
require_cmd python3
require_cmd ffprobe

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

wait_until() {
  local desc="$1"
  local timeout_sec="$2"
  shift 2
  local start
  start=$(date +%s)
  while true; do
    if "$@"; then
      return 0
    fi
    if [[ $(($(date +%s) - start)) -ge "${timeout_sec}" ]]; then
      echo "Timeout waiting: ${desc}" >&2
      return 1
    fi
    sleep 3
  done
}

wait_file_nonempty() {
  local path="$1"
  local timeout_sec="${2:-60}"
  wait_until "non-empty file ${path}" "${timeout_sec}" test -s "${path}"
}

# --- prechecks ---
if ! ds_ready; then
  echo "DeepStream is not ds-ready. Start kafka + deepstream and wait for TensorRT / pipeline." >&2
  exit 1
fi

if ! camera_in_pipeline; then
  echo "Camera_id=${CAMERA_ID} not in stream-info. Add the stream first (REST stream/add)." >&2
  exit 1
fi

mkdir -p "${STORAGE}/${CAMERA_ID}/rolling" "${STORAGE}/${CAMERA_ID}/screenshots" "${STORAGE}/${CAMERA_ID}/locked"

echo "=> [1/6] Kafka: start_rolling (auto / rolling archive)"
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'start_rolling','source_id':'${CAMERA_ID}'}))")"

echo "=> [2/6] Wait for at least one rolling segment under storage ..."
wait_until "first rolling MP4" "${WAIT_FIRST_SEGMENT_SEC}" bash -c 'n=$(find "'"${STORAGE}/${CAMERA_ID}"'/rolling" -maxdepth 1 -name "*.mp4" -type f 2>/dev/null | wc -l); [[ "${n}" -ge 1 ]]'

ROLL_FILES="$(find "${STORAGE}/${CAMERA_ID}/rolling" -maxdepth 1 -name '*.mp4' -type f 2>/dev/null | sort)"
echo "Rolling files:"
echo "${ROLL_FILES}" | sed 's/^/  /'

FIRST_MP4="$(echo "${ROLL_FILES}" | head -1)"
test -n "${FIRST_MP4}" && test -s "${FIRST_MP4}"

echo "=> [3/6] Kafka: screenshot"
SNAP_NAME="kafka_e2e_$(date -u +%Y%m%dT%H%M%SZ).jpg"
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'screenshot','source_id':'${CAMERA_ID}','filename':'${SNAP_NAME}'}))")"
SNAP_PATH="${STORAGE}/${CAMERA_ID}/screenshots/${SNAP_NAME}"
wait_file_nonempty "${SNAP_PATH}" 45
echo "Screenshot OK: ${SNAP_PATH}"

echo "=> [4/6] Kafka: manual recording (single rolling segment trim)"
read -r REQ1 START_ISO END_ISO <<<"$(python3 <<PY
import json, subprocess, uuid
from pathlib import Path
from datetime import datetime, timedelta, timezone

cam = "${CAMERA_ID}"
store = Path("${STORAGE}")
rolling = store / cam / "rolling"
files = sorted(rolling.glob("*.mp4"), key=lambda p: p.stat().st_mtime)
if not files:
    raise SystemExit("no rolling mp4")
path = files[0]
dur = float(subprocess.check_output(
    ["ffprobe", "-v", "error", "-show_entries", "format=duration",
     "-of", "default=noprint_wrappers=1:nokey=1", str(path)],
    text=True,
).strip())
mtime = path.stat().st_mtime
wall_end = datetime.fromtimestamp(mtime, tz=timezone.utc)
wall_start = wall_end - timedelta(seconds=dur)
# Interior window ~30% in the middle of the segment
span = max(dur * 0.3, 2.0)
mid = wall_start + timedelta(seconds=(dur - span) / 2)
ws = mid
we = mid + timedelta(seconds=min(span, dur - 1.0))
if we <= ws:
    we = wall_end - timedelta(seconds=1)
    ws = wall_start + timedelta(seconds=1)
rid = str(uuid.uuid4())
print(rid, ws.strftime("%Y-%m-%dT%H:%M:%SZ"), we.strftime("%Y-%m-%dT%H:%M:%SZ"))
PY
)"

kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'start_recording','source_id':'${CAMERA_ID}','request_id':'${REQ1}','start_ts':'${START_ISO}'}))")"
sleep 1
kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'stop_recording','source_id':'${CAMERA_ID}','request_id':'${REQ1}','end_ts':'${END_ISO}'}))")"

CLIP1="${STORAGE}/${CAMERA_ID}/locked/clip_${REQ1}.mp4"
wait_until "locked clip (single segment) ${CLIP1}" "${WAIT_CLIP_SEC}" test -s "${CLIP1}"
echo "Manual clip (single) OK: ${CLIP1}"

echo "=> [5/6] Wait for a second rolling segment (concat path) ..."
if ! wait_until "second rolling MP4" "${WAIT_SECOND_SEGMENT_SEC}" bash -c 'n=$(find "'"${STORAGE}/${CAMERA_ID}"'/rolling" -maxdepth 1 -name "*.mp4" -type f 2>/dev/null | wc -l); [[ "${n}" -ge 2 ]]'; then
  echo "WARN: Fewer than 2 rolling segments before timeout. Skipping multi-segment concat test." >&2
  echo "    Hint: lower DS_RECORDING_SEGMENT_SEC and recreate the deepstream container, then re-run." >&2
else
  echo "=> [6/6] Kafka: manual recording (two segments -> ffmpeg concat)"
  read -r REQ2 START2_ISO END2_ISO <<<"$(CAMERA_ID="${CAMERA_ID}" STORAGE="${STORAGE}" python3 <<'PY'
import json, os, subprocess, uuid
from pathlib import Path
from datetime import datetime, timedelta, timezone

cam = os.environ["CAMERA_ID"]
store = Path(os.environ["STORAGE"])
rolling = store / cam / "rolling"
files = sorted(rolling.glob("*.mp4"), key=lambda p: p.stat().st_mtime)
if len(files) < 2:
    raise SystemExit("need 2 rolling mp4")


def wall_range(path: Path):
    dur = float(subprocess.check_output(
        ["ffprobe", "-v", "error", "-show_entries", "format=duration",
         "-of", "default=noprint_wrappers=1:nokey=1", str(path)],
        text=True,
    ).strip())
    mtime = path.stat().st_mtime
    wall_end = datetime.fromtimestamp(mtime, tz=timezone.utc)
    wall_start = wall_end - timedelta(seconds=dur)
    return wall_start, wall_end

a, b = files[0], files[1]
ws_a, we_a = wall_range(a)
ws_b, we_b = wall_range(b)
# Wall-clock window overlapping both segments (matches RollingClipExtractor overlap rules)
start = ws_a + timedelta(seconds=2)
end = we_b - timedelta(seconds=2)
if end <= start:
    raise SystemExit("cannot build valid two-segment window (segments too short?)")

rid = str(uuid.uuid4())
print(rid, start.strftime("%Y-%m-%dT%H:%M:%SZ"), end.strftime("%Y-%m-%dT%H:%M:%SZ"))
PY
)"

  kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'start_recording','source_id':'${CAMERA_ID}','request_id':'${REQ2}','start_ts':'${START2_ISO}'}))")"
  sleep 1
  kafka_send_json "$(python3 -c "import json; print(json.dumps({'action':'stop_recording','source_id':'${CAMERA_ID}','request_id':'${REQ2}','end_ts':'${END2_ISO}'}))")"

  CLIP2="${STORAGE}/${CAMERA_ID}/locked/clip_${REQ2}.mp4"
  wait_until "locked clip (concat) ${CLIP2}" "${WAIT_CLIP_SEC}" test -s "${CLIP2}"
  echo "Manual clip (concat) OK: ${CLIP2}"
fi

echo
echo "=== Storage verification (${STORAGE}/${CAMERA_ID}) ==="
echo "--- rolling/ ---"
ls -la "${STORAGE}/${CAMERA_ID}/rolling" 2>/dev/null || true
echo "--- screenshots/ ---"
ls -la "${STORAGE}/${CAMERA_ID}/screenshots" 2>/dev/null || true
echo "--- locked/ ---"
ls -la "${STORAGE}/${CAMERA_ID}/locked" 2>/dev/null || true

echo
echo "All requested checks completed. Review directories above."
