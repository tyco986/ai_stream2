# DeepStream API Test Guide

This directory contains black-box tests for DeepStream REST APIs and Kafka command channel APIs.

## Scope

- REST:
  - `health/get-dsready-state`
  - `stream/get-stream-info`
  - `stream/add`
  - `stream/remove`
- Command channel:
  - `start_rolling`
  - `stop_rolling`
  - `start_recording` (event/manual)
  - `stop_recording`
  - `screenshot`
  - `switch_preview`

## Test Scripts

- `test_health_get_dsready_state.py`
- `test_stream_get_stream_info.py`
- `test_stream_add.py`
- `test_stream_remove.py`
- `test_command_start_rolling.py`
- `test_command_stop_rolling.py`
- `test_command_start_recording_event.py`
- `test_command_start_recording_manual.py`
- `test_command_stop_recording.py`
- `test_command_screenshot.py`
- `test_command_switch_preview.py`
- `test_all.py`

## Runtime Strategy (`test_all.py`)

`test_all.py` uses a stateful orchestration strategy for stability:

1. Prepare one persistent test camera once.
2. Run command tests with `--no-prepare` to reuse the same camera.
3. Run `stream/remove` at the end.
4. Always continue all tests and print a summary.
5. Return non-zero if any case failed.

This avoids repeated add/remove churn on dynamic sources.

## Command Payload Contract

- For `start_rolling`, `stop_rolling`, `start_recording`, `stop_recording`, `screenshot`:
  - `source_id` field carries `sensor_id`/`camera_id` string.
- For `switch_preview`:
  - `source_id` is an integer (`-1` for multi-view).

## Quick Start (inside DeepStream container)

Run all tests:

```bash
python3 /app/test/test_all.py \
  --base-url http://127.0.0.1:9000 \
  --kafka-broker ai-stream2-kafka:9092 \
  --command-topic deepstream-commands \
  --camera-url file:///app/example_data/video2_bf0.mp4 \
  --timeout 40
```

Run one command test with persistent source reuse:

```bash
python3 /app/test/test_command_screenshot.py \
  --base-url http://127.0.0.1:9000 \
  --kafka-broker ai-stream2-kafka:9092 \
  --command-topic deepstream-commands \
  --camera-id test_camera \
  --camera-url file:///app/example_data/video2_bf0.mp4 \
  --timeout 40 \
  --no-prepare
```

## Common Parameters

- `--base-url`: DeepStream REST base URL.
- `--timeout`: HTTP/Kafka timeout in seconds.
- `--camera-id`: camera identifier used by tests.
- `--camera-name`: camera display name.
- `--camera-url`: input stream URL.
- `--kafka-broker`: Kafka bootstrap server (command tests only).
- `--command-topic`: Kafka command topic (command tests only).
- `--verbose`: print debug payloads.
- `--no-prepare`: skip per-script stream preparation (command tests only).

## Screenshot Compatibility Note

Different pyservicemaker builds expose different `Buffer` APIs.

- If raw JPEG extraction is supported, screenshot test writes real frame output.
- If not supported, handler writes a fallback JPEG file but keeps command/event flow intact.

This keeps command-chain verification stable across SDK variants.
