# shellcheck shell=bash
# Usage: . "$(dirname "${BASH_SOURCE[0]}")/_api_curl.sh"
# Prints JSON body on success; on HTTP >= 400 prints body + status to stderr.

api_curl() {
  local response http_code body
  response=$(curl -sS -w $'\n%{http_code}' "$@")
  http_code="${response##*$'\n'}"
  body="${response%$'\n'*}"
  if [[ "$http_code" -ge 400 ]]; then
    echo "$body" >&2
    echo "HTTP ${http_code}" >&2
    return 1
  fi
  printf '%s' "$body"
}
