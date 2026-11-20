#!/usr/bin/env bash
# Download Debian 13 (trixie) amd64 offline debs: curl, jq, Docker, NVIDIA Container Toolkit.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${ROOT}/packages/trixie-amd64-debs"
mkdir -p "${DEST}"

URLS=(
  https://deb.debian.org/debian/pool/main/c/curl/curl_8.14.1-2+deb13u7_amd64.deb
  https://deb.debian.org/debian/pool/main/c/curl/libcurl4t64_8.14.1-2+deb13u7_amd64.deb
  https://deb.debian.org/debian/pool/main/j/jq/jq_1.7.1-6+deb13u4_amd64.deb
  https://deb.debian.org/debian/pool/main/j/jq/libjq1_1.7.1-6+deb13u4_amd64.deb
  https://download.docker.com/linux/debian/dists/trixie/pool/stable/amd64/containerd.io_2.4.2-4~debian.13~trixie_amd64.deb
  https://download.docker.com/linux/debian/dists/trixie/pool/stable/amd64/docker-ce_29.10.0-1~debian.13~trixie_amd64.deb
  https://download.docker.com/linux/debian/dists/trixie/pool/stable/amd64/docker-ce-cli_29.10.0-1~debian.13~trixie_amd64.deb
  https://download.docker.com/linux/debian/dists/trixie/pool/stable/amd64/docker-buildx-plugin_0.39.1-1~debian.13~trixie_amd64.deb
  https://download.docker.com/linux/debian/dists/trixie/pool/stable/amd64/docker-compose-plugin_5.8.0-1~debian.13~trixie_amd64.deb
  https://nvidia.github.io/libnvidia-container/stable/deb/amd64/libnvidia-container1_1.20.2-1_amd64.deb
  https://nvidia.github.io/libnvidia-container/stable/deb/amd64/libnvidia-container-tools_1.20.2-1_amd64.deb
  https://nvidia.github.io/libnvidia-container/stable/deb/amd64/nvidia-container-toolkit-base_1.20.2-1_amd64.deb
  https://nvidia.github.io/libnvidia-container/stable/deb/amd64/nvidia-container-toolkit_1.20.2-1_amd64.deb
)

for url in "${URLS[@]}"; do
  curl -fL --retry 3 -o "${DEST}/$(basename "${url}")" "${url}"
done

echo "${DEST}"
