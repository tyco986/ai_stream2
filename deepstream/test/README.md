# DeepStream API Test Guide

This directory contains black-box integration tests and unit tests for the DeepStream service.

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

## Test Files

- `test_deepstream_api.py` — Integration tests (requires running DeepStream container)
- `test_unit.py` — Unit tests for StorageManager, DiskGuard, recording archival, resolve helpers (no container needed)
- `conftest.py` — pytest fixtures for integration tests
- `_common.py` — Shared helpers (HTTP, Kafka, path utilities)

## Command Payload Contract

- For `start_rolling`, `stop_rolling`, `start_recording`, `stop_recording`, `screenshot`:
  - `source_id` field carries `sensor_id`/`camera_id` string.
- For `switch_preview`:
  - `source_id` is an integer (`-1` for multi-view).

## Storage Layout

Tests expect the new per-camera storage structure:

```
storage/
├── recordings/              ← SmartRecord buffer (temporary)
├── {camera_id}/
│   ├── recordings/          ← Archived recording segments
│   └── screenshots/         ← Screenshots
```

## Quick Start

### Unit tests (local, no container)

```bash
cd deepstream
python -m pytest test/test_unit.py --noconftest -v
```

### Integration tests (inside DeepStream container)

```bash
pytest /app/test/test_deepstream_api.py -v \
  --base-url http://127.0.0.1:9000 \
  --kafka-broker kafka:9092 \
  --command-topic deepstream-commands \
  --camera-url rtsp://127.0.0.1:8554/video1
```

### Integration tests (from host)

```bash
pytest deepstream/test/test_deepstream_api.py -v \
  --base-url http://127.0.0.1:9000 \
  --kafka-broker 127.0.0.1:19092 \
  --camera-url rtsp://127.0.0.1:8555/video1
```

## CLI Options

- `--base-url`: DeepStream REST API base URL.
- `--kafka-broker`: Kafka bootstrap server.
- `--command-topic`: Kafka command topic (command tests only).
- `--camera-url`: RTSP URL of the test video stream.
- `--camera-id`: Camera sensor ID (default: `test_cam_001`).
- `--timeout`: HTTP request timeout in seconds (default: 10).

## Screenshot Compatibility Note

Different pyservicemaker builds expose different `Buffer` APIs.

- If raw JPEG extraction is supported, screenshot test writes real frame output.
- If not supported, handler writes a fallback JPEG file but keeps command/event flow intact.

This keeps command-chain verification stable across SDK variants.
