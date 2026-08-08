#!/usr/bin/env bash
# Container entry: init site age keys (once), ensure seed payload dir, then exec CMD.
set -euo pipefail

SITE_KEY="${AGE_SITE_KEY_PATH:-/secrets/age/site.key}"
SITE_PUB="${AGE_SITE_PUB_PATH:-/secrets/age/site.pub}"
PAYLOAD_DIR="${SITE_CONFIG_PAYLOAD_DIR:-/secrets/age/payloads}"

mkdir -p "$(dirname "${SITE_KEY}")" "${PAYLOAD_DIR}"

if [[ ! -f "${SITE_KEY}" ]]; then
  echo "Generating site age key at ${SITE_KEY}"
  age-keygen -o "${SITE_KEY}"
  chmod 600 "${SITE_KEY}"
  age-keygen -y "${SITE_KEY}" > "${SITE_PUB}"
  chmod 644 "${SITE_PUB}"
elif [[ ! -f "${SITE_PUB}" ]]; then
  age-keygen -y "${SITE_KEY}" > "${SITE_PUB}"
  chmod 644 "${SITE_PUB}"
fi

if [[ "${RUN_MIGRATE:-1}" == "1" ]]; then
  python manage.py migrate --noinput
  python manage.py ensure_site_config_seed
fi

exec "$@"
