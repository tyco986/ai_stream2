#!/usr/bin/env bash
# Install Debian 13 (trixie) amd64 offline debs, one package at a time.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${ROOT}/packages/trixie-amd64-debs"

PACKAGES=(
  libcurl4t64_8.14.1-2+deb13u7_amd64.deb
  curl_8.14.1-2+deb13u7_amd64.deb
  libjq1_1.7.1-6+deb13u4_amd64.deb
  jq_1.7.1-6+deb13u4_amd64.deb
  containerd.io_2.4.2-4~debian.13~trixie_amd64.deb
  docker-ce-cli_29.10.0-1~debian.13~trixie_amd64.deb
  docker-ce_29.10.0-1~debian.13~trixie_amd64.deb
  docker-buildx-plugin_0.39.1-1~debian.13~trixie_amd64.deb
  docker-compose-plugin_5.8.0-1~debian.13~trixie_amd64.deb
  libnvidia-container1_1.20.2-1_amd64.deb
  libnvidia-container-tools_1.20.2-1_amd64.deb
  nvidia-container-toolkit-base_1.20.2-1_amd64.deb
  nvidia-container-toolkit_1.20.2-1_amd64.deb
)

for name in "${PACKAGES[@]}"; do
  dpkg -i "${DEST}/${name}"
done
