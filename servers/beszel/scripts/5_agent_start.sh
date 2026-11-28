#!/usr/bin/env bash
# Container entrypoint. Hub is already up; fetch the agent key and token, then exec /agent.
set -euo pipefail

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

CREDS="$(USER_EMAIL="${USER_EMAIL}" USER_PASSWORD="${USER_PASSWORD}" HUB_URL="${HUB_URL}" python3 - <<'PY'
import json, os, time, urllib.request

base = os.environ["HUB_URL"].rstrip("/")
email = os.environ["USER_EMAIL"]
password = os.environ["USER_PASSWORD"]

def call(path, payload=None, token=""):
    data = None if payload is None else json.dumps(payload).encode()
    request = urllib.request.Request(base + path, data=data)
    request.add_header("Content-Type", "application/json")
    if token:
        request.add_header("Authorization", token)
    with urllib.request.urlopen(request, timeout=10) as response:
        return json.load(response)

ready = False
for _ in range(40):
    try:
        urllib.request.urlopen(base + "/api/health", timeout=2).read()
        ready = True
        break
    except Exception:
        time.sleep(1)
if not ready:
    raise SystemExit("hub health failed")

auth = call(
    "/api/collections/users/auth-with-password",
    {"identity": email, "password": password},
)
token = auth["token"]
key = call("/api/beszel/getkey", token=token)["key"]
universal = call("/api/beszel/universal-token?enable=1&permanent=1", token=token)["token"]
print(key)
print(universal)
PY
)"

KEY="$(sed -n '1p' <<<"${CREDS}")"
TOKEN="$(sed -n '2p' <<<"${CREDS}")"
export KEY TOKEN SMART_DEVICES
exec /agent
