# Source after ROOT is set to the repository root.
# Usage:
#   ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
#   # shellcheck source=../../../scripts/load_project_env.sh
#   source "${ROOT}/scripts/load_project_env.sh"

if [[ -z "${ROOT:-}" ]]; then
  echo "load_project_env.sh: ROOT must be set to the repository root" >&2
  return 1 2>/dev/null || exit 1
fi

if [[ ! -f "${ROOT}/project.env" ]]; then
  echo "load_project_env.sh: missing ${ROOT}/project.env" >&2
  return 1 2>/dev/null || exit 1
fi

set -a
# shellcheck disable=SC1091
source "${ROOT}/project.env"
set +a

if [[ -z "${PROJECT_NAME:-}" ]]; then
  echo "load_project_env.sh: PROJECT_NAME is empty in project.env" >&2
  return 1 2>/dev/null || exit 1
fi
