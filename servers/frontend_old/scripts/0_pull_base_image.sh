#!/usr/bin/env bash
# Pull base images used by pure-admin-thin-i18n/Dockerfile.
set -euo pipefail

docker pull node:20-alpine
docker pull nginx:stable-alpine
