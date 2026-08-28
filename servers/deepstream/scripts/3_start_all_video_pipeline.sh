#!/usr/bin/env bash
# Run from project root. Start every non-base *video* pipeline template sequentially.
# After each pipeline finishes, restart the deepstream container via docker_socket_proxy.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8092"
START_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/start_pipeline"
STATUS_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/pipeline/status"
HEALTH_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/health"
TEMPLATES_DIR="${ROOT}/servers/deepstream/templates"
WAIT_TIMEOUT_SEC="${WAIT_TIMEOUT_SEC:-600}"
POLL_INTERVAL_SEC="${POLL_INTERVAL_SEC:-2}"
DOCKER_PROXY_URL="${DOCKER_PROXY_URL:-http://127.0.0.1:2375}"
DEEPSTREAM_CONTAINER="${DEEPSTREAM_CONTAINER:-${PROJECT_NAME}_deepstream}"

usage() {
  cat <<EOF
usage: $0

Start every video pipeline YAML under servers/deepstream/templates except base/,
one after another (wait until each finishes). After each run, restart
\${DEEPSTREAM_CONTAINER} via docker_socket_proxy and wait until the API is up.

Environment:
  WAIT_TIMEOUT_SEC       Max seconds to wait per pipeline / API ready (default 600)
  POLL_INTERVAL_SEC      Status poll interval seconds (default 2)
  DOCKER_PROXY_URL       docker_socket_proxy base URL (default http://127.0.0.1:2375)
  DEEPSTREAM_CONTAINER   Container to restart (default \${PROJECT_NAME}_deepstream)

Prerequisites: deepstream on :8092, docker_socket_proxy on :2375
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

wait_until_api_ready() {
  local deadline=$((SECONDS + WAIT_TIMEOUT_SEC))
  while (( SECONDS < deadline )); do
    local body=""
    if body="$(curl -sS --connect-timeout 2 "${HEALTH_ENDPOINT}" 2>/dev/null)"; then
      if echo "${body}" | grep -q '"success"[[:space:]]*:[[:space:]]*true'; then
        return 0
      fi
    fi
    sleep "${POLL_INTERVAL_SEC}"
  done
  echo "timeout waiting for deepstream API after restart (${WAIT_TIMEOUT_SEC}s)" >&2
  return 1
}

restart_deepstream() {
  echo "restart: ${DEEPSTREAM_CONTAINER}"
  local http_code
  http_code="$(curl -sS -o /dev/null -w "%{http_code}" \
    -X POST "${DOCKER_PROXY_URL}/containers/${DEEPSTREAM_CONTAINER}/restart")"
  if [[ "${http_code}" != "204" && "${http_code}" != "200" ]]; then
    echo "FAILED restart: ${DEEPSTREAM_CONTAINER} (http ${http_code})" >&2
    return 1
  fi
  wait_until_api_ready
}

mapfile -t CONFIGS < <(
  find "${TEMPLATES_DIR}" -type f -name '*_video_pipeline*.yml' ! -path '*/base/*' | sort
)
[[ ${#CONFIGS[@]} -gt 0 ]] || { echo "no video pipeline templates found" >&2; exit 1; }

failed=0
ok=0
for config_path in "${CONFIGS[@]}"; do
  rel="${config_path#"${TEMPLATES_DIR}/"}"
  echo "==> ${rel}"
  wait_until_idle
  response_body="$(mktemp)"
  http_code="$(curl -sS -w "%{http_code}" -o "${response_body}" \
    -X POST "${START_ENDPOINT}" \
    -F "input=@${config_path}")"
  cat "${response_body}"
  echo
  rm -f "${response_body}"
  if [[ "${http_code}" != "200" ]]; then
    echo "FAILED start: ${rel} (http ${http_code})" >&2
    failed=$((failed + 1))
    restart_deepstream || true
    continue
  fi
  if wait_until_idle; then
    ok=$((ok + 1))
  else
    echo "FAILED wait: ${rel}" >&2
    failed=$((failed + 1))
  fi
  restart_deepstream || {
    failed=$((failed + 1))
    echo "FAILED after restart wait: ${rel}" >&2
  }
done

echo
echo "done: ok=${ok} failed=${failed} total=${#CONFIGS[@]}"
[[ "${failed}" -eq 0 ]]
