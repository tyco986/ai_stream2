#!/usr/bin/env bash
# Run from project root. Start every non-base *image* pipeline template sequentially.
# Waits until each pipeline finishes, then starts the next without restarting the container.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8092"
START_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/start_pipeline"
STATUS_ENDPOINT="${API_URL}/${PROJECT_NAME}/deepstream/pipeline/status"
TEMPLATES_DIR="${ROOT}/servers/deepstream/templates"
WAIT_TIMEOUT_SEC="${WAIT_TIMEOUT_SEC:-180}"
POLL_INTERVAL_SEC="${POLL_INTERVAL_SEC:-1}"

usage() {
  cat <<EOF
usage: $0

Start every image pipeline YAML under servers/deepstream/templates except base/,
one after another (wait until each finishes). The same deepstream process is reused.

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

mapfile -t CONFIGS < <(
  find "${TEMPLATES_DIR}" -type f -name '*_image_pipeline*.yml' ! -path '*/base/*' | sort
)
[[ ${#CONFIGS[@]} -gt 0 ]] || { echo "no image pipeline templates found" >&2; exit 1; }

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
    continue
  fi
  if wait_until_idle; then
    ok=$((ok + 1))
  else
    echo "FAILED wait: ${rel}" >&2
    failed=$((failed + 1))
  fi
done

echo
echo "done: ok=${ok} failed=${failed} total=${#CONFIGS[@]}"
[[ "${failed}" -eq 0 ]]
