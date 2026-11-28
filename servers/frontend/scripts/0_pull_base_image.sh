#!/usr/bin/env bash
# Pull the base image used by servers/frontend/Dockerfile.
set -euo pipefail

docker pull node:22-bookworm-slim
