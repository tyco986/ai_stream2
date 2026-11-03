#!/usr/bin/env bash
# Run from project root. Generate all configs under servers/generator/templates.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8091"
ENDPOINT="${API_URL}/${PROJECT_NAME}/generator/generate"
TEMPLATES_DIR="${ROOT}/servers/generator/templates"

usage() {
  cat <<EOF
usage: $0

Generate DeepStream pipeline configs for every YAML under servers/generator/templates.

Prerequisites: 1_build_dev_image.sh or 1_build_prod_image.sh, 2_run_dev_container.sh or 2_run_prod_container.sh
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

mapfile -t CONFIGS < <(find "${TEMPLATES_DIR}" -type f -name '*.yaml' | sort)
[[ ${#CONFIGS[@]} -gt 0 ]] || { echo "no templates found under ${TEMPLATES_DIR}" >&2; exit 1; }

failed=0
ok=0
for config_path in "${CONFIGS[@]}"; do
  rel="${config_path#"${TEMPLATES_DIR}/"}"
  echo "==> ${rel}"
  response_body="$(mktemp)"
  http_code="$(curl -sS -w "%{http_code}" -o "${response_body}" \
    -X POST "${ENDPOINT}" \
    -F "input=@${config_path}")"
  cat "${response_body}"
  echo
  rm -f "${response_body}"
  if [[ "${http_code}" == "200" ]]; then
    ok=$((ok + 1))
  else
    echo "FAILED: ${rel} (http ${http_code})" >&2
    failed=$((failed + 1))
  fi
done

echo
echo "done: ok=${ok} failed=${failed} total=${#CONFIGS[@]}"
[[ "${failed}" -eq 0 ]]
