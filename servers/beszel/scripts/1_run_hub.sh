#!/usr/bin/env bash
# Run from project root. Start Beszel Hub (web UI).
# Docker Desktop/WSL: publish a host port (default 8093; 8090 is used by export_onnx).
set -euo pipefail

PROJECT_NAME=ai_stream2

NAME="${BESZEL_HUB_NAME:-${PROJECT_NAME}_beszel}"
AGENT_NAME="${BESZEL_AGENT_NAME:-${PROJECT_NAME}_beszel_localhost}"
NET="${BESZEL_NETWORK:-${PROJECT_NAME}_default}"
IMG="${BESZEL_HUB_IMAGE:-henrygd/beszel:latest}"
AGENT_IMG="${BESZEL_AGENT_IMAGE:-henrygd/beszel-agent-nvidia:latest}"
HOST_PORT="${BESZEL_HOST_PORT:-8093}"
VOL_DATA="${PROJECT_NAME}_beszel_data"
VOL_AGENT="${PROJECT_NAME}_beszel_agent_data"
APP_URL="${BESZEL_APP_URL:-http://127.0.0.1:${HOST_PORT}}"
USER_EMAIL=admin@example.com
USER_PASSWORD=88888888
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

# Hub: start the UI, log in as a users record, and fetch the agent key and token.
docker network create "${NET}" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true
docker volume rm "${VOL_DATA}" >/dev/null 2>&1 || true
docker volume create "${VOL_DATA}" >/dev/null

docker run -d \
  --name "${NAME}" \
  --ulimit core=0 \
  --network "${NET}" \
  --restart unless-stopped \
  -e "APP_URL=${APP_URL}" \
  -e "USER_EMAIL=${USER_EMAIL}" \
  -e "USER_PASSWORD=${USER_PASSWORD}" \
  -p "${HOST_PORT}:8090" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOL_DATA}:/beszel_data" \
  "${IMG}"

AUTH_BODY="$(printf '{"identity":"%s","password":"%s"}' "${USER_EMAIL}" "${USER_PASSWORD}")"
for _ in $(seq 1 40); do
  curl -sf "${APP_URL}/api/health" >/dev/null && break
  sleep 1
done

AUTH_CODE="$(curl -sS -o /tmp/beszel_hub_auth.json -w '%{http_code}' \
  -X POST "${APP_URL}/api/collections/users/auth-with-password" \
  -H 'Content-Type: application/json' \
  -d "${AUTH_BODY}")"
if [[ "${AUTH_CODE}" != "200" ]]; then
  echo "Hub auth failed (${AUTH_CODE}): $(cat /tmp/beszel_hub_auth.json)" >&2
  exit 1
fi
TOKEN_JSON="$(cat /tmp/beszel_hub_auth.json)"

AUTH_TOKEN="$(jq -r '.token' <<<"${TOKEN_JSON}")"
AUTH_HEADER=(-H "Authorization: ${AUTH_TOKEN}")

KEY_JSON="$(curl -sf "${APP_URL}/api/beszel/getkey" "${AUTH_HEADER[@]}")"
BESZEL_KEY="$(jq -r '.key' <<<"${KEY_JSON}")"

UNIVERSAL_JSON="$(curl -sf "${APP_URL}/api/beszel/universal-token?enable=1&permanent=1" "${AUTH_HEADER[@]}")"
BESZEL_TOKEN="$(jq -r '.token' <<<"${UNIVERSAL_JSON}")"

# Agent: connect with the universal token and register as localhost.
docker volume create "${VOL_AGENT}" >/dev/null
docker rm -f "${AGENT_NAME}" "${PROJECT_NAME}_beszel_agent" 2>/dev/null || true

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
  --name "${AGENT_NAME}" \
  --ulimit core=0 \
  --network "${NET}" \
  --restart unless-stopped \
  "${GPU_ARGS[@]}" \
  "${SMART_ARGS[@]}" \
  -e "HUB_URL=http://${NAME}:8090" \
  -e "TOKEN=${BESZEL_TOKEN}" \
  -e "KEY=${BESZEL_KEY}" \
  -e "SYSTEM_NAME=${SYSTEM_NAME}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${VOL_AGENT}:/var/lib/beszel-agent" \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  "${AGENT_IMG}"

cat <<EOF
${NAME} (${NET})
  UI: ${APP_URL}
  image: ${IMG}
  login: ${USER_EMAIL} / ${USER_PASSWORD}
  system: ${SYSTEM_NAME}
${AGENT_NAME}
  image: ${AGENT_IMG}
  gpu: $([ ${#GPU_ARGS[@]} -gt 0 ] && echo enabled || echo disabled)
  smart: ${SMART_DEVICES:-disabled}
EOF
