#!/usr/bin/env bash
# Run from project root. Exec migrate in the running django container.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

docker exec "${PROJECT_NAME}_django" python manage.py migrate --noinput
