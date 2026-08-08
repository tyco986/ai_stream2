#!/usr/bin/env sh
set -eu

vite --mode real --host --port 5173 &
real_pid=$!
vite --mode mock --host --port 5174 &
mock_pid=$!

term() {
  kill "$real_pid" "$mock_pid" 2>/dev/null || true
  wait "$real_pid" "$mock_pid" 2>/dev/null || true
}

trap term INT TERM EXIT
wait "$real_pid" "$mock_pid"
