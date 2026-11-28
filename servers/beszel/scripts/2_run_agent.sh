#!/usr/bin/env bash
# Run from project root. Start Beszel Agent and register it as localhost.
# Requires TOKEN/KEY from Hub UI (Add System), or universal token from Settings → Tokens.
#
#   export BESZEL_TOKEN='...'
#   export BESZEL_KEY='ssh-ed25519 ...'
#   bash servers/beszel/scripts/2_run_agent.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

NAME="${BESZEL_AGENT_NAME:-${PROJECT_NAME}_beszel_localhost}"
HUB_NAME="${BESZEL_HUB_NAME:-${PROJECT_NAME}_beszel}"
NET="${BESZEL_NETWORK:-${PROJECT_NAME}}"
IMG="${BESZEL_AGENT_IMAGE:-henrygd/beszel-agent-nvidia:latest}"
VOL_AGENT="${PROJECT_NAME}_beszel_agent"
SYSTEM_NAME=localhost
SMART_DEVICES="$(lsblk -dn -o NAME,TYPE,TRAN | awk '
$2 != "disk" { next }
$3 != "nvme" && $3 != "sata" && $3 != "ata" && $3 != "sas" && $3 != "usb" { next }
{
  name = $1
  vendor = model = ""
  getline vendor < ("/sys/block/" name "/device/vendor")
  getline model < ("/sys/block/" name "/device/model")
  close("/sys/block/" name "/device/vendor")
  close("/sys/block/" name "/device/model")
  ident = tolower(vendor model)
  if (ident ~ /msft|qemu|vmware|virtual/) { next }
  type = "scsi"
  if ($3 == "nvme") { type = "nvme" }
  if ($3 == "sata" || $3 == "ata" || $3 == "usb") { type = "sat" }
  dev = "/dev/" name
  if (name ~ /^nvme[0-9]+n[0-9]+$/) {
    sub(/n[0-9]+$/, "", name)
    dev = "/dev/" name
  }
  if (seen[dev]++) { next }
  out = out (out ? "," : "") dev ":" type
}
END { printf "%s", out }
')"
HUB_URL="${BESZEL_HUB_URL:-http://${HUB_NAME}:8090}"
TOKEN="${BESZEL_TOKEN:-}"
KEY="${BESZEL_KEY:-}"

if [[ -z "${TOKEN}" || -z "${KEY}" ]]; then
  echo "BESZEL_TOKEN and BESZEL_KEY are required (from Hub Add System / Settings → Tokens)." >&2
  exit 1
fi

docker network create "${NET}" 2>/dev/null || true
docker volume create "${VOL_AGENT}" >/dev/null
docker rm -f "${NAME}" "${PROJECT_NAME}_beszel_agent" 2>/dev/null || true

GPU_ARGS=()
if command -v nvidia-smi >/dev/null 2>&1; then
  GPU_ARGS=(--gpus "all,capabilities=utility")
fi
SMART_ARGS=()
if [[ -n "${SMART_DEVICES}" ]]; then
  SMART_ARGS=(-e "SMART_DEVICES=${SMART_DEVICES}" --cap-add SYS_RAWIO)
  IFS=',' read -r -a SMART_LIST <<< "${SMART_DEVICES}"
  for device in "${SMART_LIST[@]}"; do
    SMART_ARGS+=(--device "${device%%:*}")
  done
fi

docker run -d \
  --name "${NAME}" \
  --ulimit core=0 \
  --network "${NET}" \
  --restart unless-stopped \
  "${GPU_ARGS[@]}" \
  "${SMART_ARGS[@]}" \
  -e "HUB_URL=${HUB_URL}" \
  -e "TOKEN=${TOKEN}" \
  -e "KEY=${KEY}" \
  -e "SYSTEM_NAME=${SYSTEM_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOL_AGENT}:/var/lib/beszel-agent" \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  "${IMG}"

cat <<EOF
${NAME} (${NET})
  hub: ${HUB_URL}
  system: ${SYSTEM_NAME}
  image: ${IMG}
  gpu: $([ ${#GPU_ARGS[@]} -gt 0 ] && echo enabled || echo disabled)
  smart: ${SMART_DEVICES:-disabled}
EOF
