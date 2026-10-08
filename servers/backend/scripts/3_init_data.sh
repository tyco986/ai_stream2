#!/usr/bin/env bash
# Drop public and reapply 001_init.sql. Does not start the backend.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

CONTAINER="${PROJECT_NAME}_postgresql"
SQL="${ROOT}/servers/backend/src/db/migrations/001_init.sql"

docker exec -i "${CONTAINER}" \
  psql -v ON_ERROR_STOP=1 -U "${PROJECT_NAME}" -d "${PROJECT_NAME}" \
  -c "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"

docker exec -i "${CONTAINER}" \
  psql -v ON_ERROR_STOP=1 -U "${PROJECT_NAME}" -d "${PROJECT_NAME}" \
  < "${SQL}"

docker exec -i "${CONTAINER}" \
  psql -v ON_ERROR_STOP=1 -U "${PROJECT_NAME}" -d "${PROJECT_NAME}" \
  -c "CREATE TABLE schema_migrations (version VARCHAR PRIMARY KEY); INSERT INTO schema_migrations (version) VALUES ('001_init.sql');"
