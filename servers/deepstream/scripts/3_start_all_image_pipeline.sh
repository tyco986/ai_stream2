#!/usr/bin/env bash
# Run from project root. Start every image config under configs/generator sequentially.
# Waits until each pipeline finishes, then starts the next without restarting the container.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8092"
START_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/start_pipeline"
STATUS_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/pipeline/status"
HEALTH_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/health"
CONFIGS_DIR="${ROOT}/configs/generator"
CONTAINER_CONFIGS_DIR="/root/configs/generator"
WAIT_TIMEOUT_SEC="${WAIT_TIMEOUT_SEC:-180}"
POLL_INTERVAL_SEC="${POLL_INTERVAL_SEC:-1}"

usage() {
  cat <<EOF
usage: $0

Start every image pipeline under configs/generator (dirs whose name contains
_image, with pipeline.yml and params.yml), one after another. The same
deepstream process is reused.

Environment:
  WAIT_TIMEOUT_SEC       Max seconds to wait per pipeline (default 180)
  POLL_INTERVAL_SEC      Status poll interval seconds (default 1)

Prerequisites: deepstream on :8092
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
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

wait_until_idle() {
  local deadline=$((SECONDS + WAIT_TIMEOUT_SEC))
  while (( SECONDS < deadline )); do
    local body
    body="$(curl -sS "${STATUS_ENDPOINT}")"
    if echo "${body}" | grep -q '"pipeline_running"[[:space:]]*:[[:space:]]*false'; then
      return 0
    fi
    sleep "${POLL_INTERVAL_SEC}"
  done
  echo "timeout waiting for pipeline idle (${WAIT_TIMEOUT_SEC}s)" >&2
  return 1
}

curl -sS --connect-timeout 2 "${HEALTH_ENDPOINT}" >/dev/null \
  || { echo "deepstream_api not ready: ${HEALTH_ENDPOINT}" >&2; exit 1; }

mapfile -t CONFIGS < <(
  find "${CONFIGS_DIR}" -mindepth 1 -maxdepth 1 -type d -name '*_image*' \
    | sort
)
[[ ${#CONFIGS[@]} -gt 0 ]] || { echo "no image pipeline configs found" >&2; exit 1; }

failed=0
ok=0
skipped=0
for config_path in "${CONFIGS[@]}"; do
  name="$(basename "${config_path}")"
  params_path="${config_path}/params.yml"
  pipeline_path="${config_path}/pipeline.yml"
  if [[ ! -f "${params_path}" || ! -f "${pipeline_path}" ]]; then
    echo "SKIP missing pipeline.yml or params.yml: ${name}" >&2
    skipped=$((skipped + 1))
    continue
  fi
  echo "==> ${name}"
  if ! wait_until_idle; then
    echo "FAILED idle before start: ${name}" >&2
    failed=$((failed + 1))
    continue
  fi
  response_body="$(mktemp)"
  http_code="$(curl -sS -w "%{http_code}" -o "${response_body}" \
    -X POST "${START_ENDPOINT}" \
    -H "Content-Type: application/json" \
    -d "{\"config_dir\":\"${CONTAINER_CONFIGS_DIR}/${name}\"}")"
  cat "${response_body}"
  echo
  rm -f "${response_body}"
  if [[ "${http_code}" != "200" ]]; then
    echo "FAILED start: ${name} (http ${http_code})" >&2
    failed=$((failed + 1))
    continue
  fi
  if wait_until_idle; then
    ok=$((ok + 1))
  else
    echo "FAILED wait: ${name}" >&2
    failed=$((failed + 1))
  fi
done

echo
echo "done: ok=${ok} failed=${failed} skipped=${skipped} total=${#CONFIGS[@]}"
[[ "${failed}" -eq 0 ]]
