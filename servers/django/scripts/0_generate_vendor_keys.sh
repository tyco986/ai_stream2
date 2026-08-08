#!/usr/bin/env bash
# Generate vendor age + ticket keypairs inside a one-off container.
# Private keys → ${ROOT}/secrets/vendor/ (host volume; not in image).
# Public keys  → servers/django/keys/*.pub (stable; copied into image).
# Re-run is a no-op if private keys already exist (unless FORCE=1).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

VENDOR_DIR="${ROOT}/secrets/vendor"
KEYS_DIR="${ROOT}/servers/django/keys"
BASE_IMAGE="${VENDOR_KEYS_BASE_IMAGE:-python:3.12-slim}"
FORCE="${FORCE:-0}"

mkdir -p "${VENDOR_DIR}" "${KEYS_DIR}"

if [[ "${FORCE}" != "1" && -f "${VENDOR_DIR}/dev.key" && -f "${VENDOR_DIR}/ticket.key" ]]; then
  echo "Vendor private keys already exist under ${VENDOR_DIR}; refreshing public keys only."
fi

docker run --rm \
  -v "${VENDOR_DIR}:/vendor" \
  -v "${KEYS_DIR}:/pubs" \
  -e FORCE="${FORCE}" \
  "${BASE_IMAGE}" \
  bash -c '
    set -euo pipefail
    export DEBIAN_FRONTEND=noninteractive
    sed -i "s|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g" /etc/apt/sources.list.d/debian.sources
    apt-get update
    apt-get install -y --no-install-recommends age openssl ca-certificates
    rm -rf /var/lib/apt/lists/*

    if [[ "${FORCE}" == "1" || ! -f /vendor/dev.key ]]; then
      age-keygen -o /vendor/dev.key
      chmod 600 /vendor/dev.key
    fi
    age-keygen -y /vendor/dev.key > /pubs/dev.pub
    chmod 644 /pubs/dev.pub

    if [[ "${FORCE}" == "1" || ! -f /vendor/ticket.key ]]; then
      openssl genpkey -algorithm Ed25519 -out /vendor/ticket.key
      chmod 600 /vendor/ticket.key
    fi
    openssl pkey -in /vendor/ticket.key -pubout -out /pubs/ticket.pub
    chmod 644 /pubs/ticket.pub

    echo "Wrote /pubs/dev.pub and /pubs/ticket.pub"
    echo "Private keys kept under /vendor (host: secrets/vendor)"
  '

echo "Vendor pubs: ${KEYS_DIR}/dev.pub ${KEYS_DIR}/ticket.pub"
echo "Vendor secrets: ${VENDOR_DIR}/ (gitignored)"
