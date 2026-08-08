#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

docker rm -f "${PROJECT_NAME}_postgresql" 2>/dev/null || true
echo "Stopped ${PROJECT_NAME}_postgresql"
