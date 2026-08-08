#!/usr/bin/env bash
# Run makemigrations inside the running django container (dev mount).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

docker exec "${PROJECT_NAME}_django" python manage.py makemigrations users shell
docker exec "${PROJECT_NAME}_django" python manage.py migrate --noinput
docker exec "${PROJECT_NAME}_django" python manage.py ensure_site_config_seed
