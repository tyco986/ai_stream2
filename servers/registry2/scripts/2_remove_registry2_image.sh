#!/usr/bin/env bash
# Run from project root. Delete one tag, every platform manifest under it, then unreferenced blobs.
set -euo pipefail

PROJECT_NAME=ai_stream2
CONTAINER="${PROJECT_NAME}_registry2"
VOLUME="${PROJECT_NAME}_registry2"
BASE="http://127.0.0.1:5000"
ACCEPT="application/vnd.docker.distribution.manifest.list.v2+json, application/vnd.oci.image.index.v1+json, application/vnd.docker.distribution.manifest.v2+json, application/vnd.oci.image.manifest.v1+json"

if [[ $# -ne 1 || "$1" != *:* ]]; then
  echo "usage: $0 <name:tag>" >&2
  exit 1
fi

NAME="${1%:*}"
TAG="${1##*:}"
HEADERS="$(mktemp)"
BODY="$(mktemp)"
trap 'rm -f "${HEADERS}" "${BODY}"' EXIT

curl -fsS -D "${HEADERS}" -o "${BODY}" \
  -H "Accept: ${ACCEPT}" \
  "${BASE}/v2/${NAME}/manifests/${TAG}"
DIGEST="$(awk 'tolower($1)=="docker-content-digest:" {print $2}' "${HEADERS}" | tr -d '\r')"
if [[ -z "${DIGEST}" ]]; then
  echo "missing digest for ${NAME}:${TAG}" >&2
  exit 1
fi

curl -fsS -X DELETE "${BASE}/v2/${NAME}/manifests/${DIGEST}"
if grep -q '"manifests":\[' "${BODY}"; then
  while IFS= read -r CHILD; do
    curl -fsS -X DELETE "${BASE}/v2/${NAME}/manifests/${CHILD}"
  done < <(grep -o 'sha256:[a-f0-9]\{64\}' "${BODY}" | sort -u)
fi

docker stop "${CONTAINER}"
set +e
docker run --rm \
  -v "${VOLUME}:/var/lib/registry" \
  registry:2 \
  garbage-collect --delete-untagged /etc/docker/registry/config.yml
STATUS=$?
set -e
docker start "${CONTAINER}"
if [[ "${STATUS}" -ne 0 ]]; then
  exit "${STATUS}"
fi
echo "removed ${NAME}:${TAG}"
