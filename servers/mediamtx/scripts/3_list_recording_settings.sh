#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

curl -s http://127.0.0.1:9997/v3/config/paths/list | jq '.items[] | select(.name != "all_others") | {name, record, recordPath, recordSegmentDuration, recordDeleteAfter}'
