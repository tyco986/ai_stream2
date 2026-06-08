#!/usr/bin/env bash
# Run on host: docker login nvcr.io and pull TAO training base image.
#
# Usage: ./0_pull_base_image.sh --ngc-api-key <KEY>
#        NGC_API_KEY=<KEY> ./0_pull_base_image.sh
set -euo pipefail

TAO_IMAGE="${TAO_IMAGE:-nvcr.io/nvidia/tao/tao-toolkit:6.0.0-pyt}"

NGC_API_KEY_ARG=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --ngc-api-key) NGC_API_KEY_ARG="${2:?}"; shift 2 ;;
    *) echo "Usage: $0 --ngc-api-key <KEY>" >&2; exit 1 ;;
  esac
done
NGC_API_KEY="${NGC_API_KEY_ARG:-${NGC_API_KEY:-}}"
[[ -n "${NGC_API_KEY}" ]] || { echo "ERROR: --ngc-api-key or NGC_API_KEY required" >&2; exit 1; }

command -v docker >/dev/null || { echo "ERROR: docker required" >&2; exit 1; }

echo "=> docker login nvcr.io"
docker login nvcr.io -u '$oauthtoken' -p "${NGC_API_KEY}"

echo "=> docker pull ${TAO_IMAGE}"
docker pull "${TAO_IMAGE}"

echo "=> done"
docker images "${TAO_IMAGE}"
