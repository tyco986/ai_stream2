#!/bin/sh
# Follow ai_stream2_* streams with consumer group "logger" and append daily JSONL.

LOG_DIR=/logs/redis
GROUP=logger
MAX_DAYS=90
POLL_SEC=0.2

LUA='
local pattern = "ai_stream2_*"
local group = "logger"
local consumer = "logger"
local field = "metadata"
local count = "32"
local cursor = "0"
local keys = {}
repeat
  local reply = redis.call("SCAN", cursor, "MATCH", pattern, "COUNT", 100)
  cursor = reply[1]
  local batch = reply[2]
  for index = 1, #batch do
    keys[#keys + 1] = batch[index]
  end
until cursor == "0"
for index = 1, #keys do
  pcall(redis.call, "XGROUP", "CREATE", keys[index], group, "0")
end
local function append_entries(id, out)
  if #keys == 0 then
    return
  end
  local args = {"XREADGROUP", "GROUP", group, consumer, "COUNT", count, "STREAMS"}
  for index = 1, #keys do
    args[#args + 1] = keys[index]
  end
  for index = 1, #keys do
    args[#args + 1] = id
  end
  local reply = redis.call(unpack(args))
  if not reply then
    return
  end
  for stream_index = 1, #reply do
    local name = reply[stream_index][1]
    local entries = reply[stream_index][2]
    for entry_index = 1, #entries do
      local entry = entries[entry_index]
      local entry_id = entry[1]
      local fields = entry[2]
      local payload = "null"
      for field_index = 1, #fields, 2 do
        if fields[field_index] == field then
          payload = fields[field_index + 1]
        end
      end
      if payload == "" then
        payload = "null"
      end
      local line = string.format("{\"topic\":\"%s\",\"id\":\"%s\",\"payload\":%s}", name, entry_id, payload)
      out[#out + 1] = name .. "\t" .. entry_id .. "\t" .. line
    end
  end
end
local out = {}
append_entries("0", out)
append_entries(">", out)
return out
'

mkdir -p "$LOG_DIR"
day=""
tab=$(printf '\t')
while true; do
  today=$(date +%Y-%m-%d)
  if [ "$today" != "$day" ]; then
    day=$today
    find "$LOG_DIR" -type f -name 'redis_*.jsonl' -mtime +"$MAX_DAYS" -delete 2>/dev/null || true
  fi
  file="$LOG_DIR/redis_${day}.jsonl"
  redis-cli --raw EVAL "$LUA" 0 | while IFS= read -r line; do
    if [ -n "$line" ]; then
      topic=${line%%"$tab"*}
      rest=${line#*"$tab"}
      id=${rest%%"$tab"*}
      json=${rest#*"$tab"}
      printf '%s\n' "$json" >> "$file"
      redis-cli XACK "$topic" "$GROUP" "$id" >/dev/null || true
    fi
  done
  sleep "$POLL_SEC"
done
