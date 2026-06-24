#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

HOST=127.0.0.1
PORT=8554
PL=$(curl -s http://127.0.0.1:9997/v3/paths/list)
CL=$(curl -s http://127.0.0.1:9997/v3/config/paths/list)
jq -r -n --argjson pl "$PL" --argjson cl "$CL" --arg h "$HOST" --arg p "$PORT" '
  ([$pl.items // [] | .[] | select(.online == true) | .name]) as $alive |
  $cl.items // [] | .[] | select(.name != "all_others") |
  .name as $n |
  (if .source == "publisher" then "rtsp://\($h):\($p)/\($n)" else .source end) as $url |
  ($alive | any(. == $n)) as $on |
  "\($url)\talive=\($on)"
'