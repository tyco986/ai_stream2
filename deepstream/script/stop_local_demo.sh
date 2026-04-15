#!/usr/bin/env bash
# Stops video2rtsp started by start_local_demo.sh (SIGTERM so ffmpeg subprocesses exit cleanly).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PID_FILE="${PROJECT_ROOT}/deepstream/storage/.video2rtsp.pid"

if [[ -f "${PID_FILE}" ]]; then
  pid="$(cat "${PID_FILE}")"
  if kill -0 "${pid}" 2>/dev/null; then
    echo "Sending SIGTERM to video2rtsp PID ${pid} ..."
    kill -TERM "${pid}" 2>/dev/null || true
    sleep 1
  fi
  rm -f "${PID_FILE}"
fi

if pgrep -f "${PROJECT_ROOT}/deepstream/script/video2rtsp.py" >/dev/null 2>&1; then
  echo "Cleaning up stray video2rtsp.py processes ..."
  pkill -TERM -f "${PROJECT_ROOT}/deepstream/script/video2rtsp.py" || true
fi

echo "Stopped. (DeepStream containers were not stopped; use: docker compose stop deepstream kafka)"
