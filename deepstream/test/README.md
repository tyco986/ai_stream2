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
  - `start_recording` / `stop_recording` (UTC window: `request_id`, `start_ts`, `end_ts`)
  - `screenshot`
  - `switch_preview`

## Test Files

- `test_deepstream_api.py` — Integration tests (requires running DeepStream container)
- `test_unit.py` — Unit tests for StorageManager, DiskGuard, recording archival, resolve helpers (no container needed)
- `test_clip_extraction_e2e.py` — End-to-end clip extraction: generates real MP4s in `rolling/`, runs `RollingClipExtractor`, asserts output under `locked/` (requires **ffmpeg** + **ffprobe** on `PATH` or `/usr/local/bin/`)
- `conftest.py` — pytest fixtures for integration tests
- `_common.py` — Shared helpers (HTTP, Kafka, path utilities)

## Command Payload Contract

- For `start_rolling`, `stop_rolling`, `screenshot`:
  - `source_id` field carries `sensor_id`/`camera_id` string.
- For `start_recording` / `stop_recording` (rolling → locked clip extraction):
  - `source_id`, `request_id` (pairing), `start_ts` / `end_ts` (ISO8601 UTC).
- For `switch_preview`:
  - `source_id` is an integer (`-1` for multi-view).

## Storage Layout

Tests expect the new per-camera storage structure:

```
storage/
├── recordings/              ← SmartRecord buffer (temporary)
├── {camera_id}/
│   ├── rolling/             ← Archived rolling segments
│   ├── locked/              ← Clips from start_recording/stop_recording window
│   └── screenshots/         ← Screenshots
```

## Quick Start

### Unit tests (local, no container)

```bash
cd deepstream
python -m pytest test/test_unit.py --noconftest -v
```

### Clip e2e (local, requires ffmpeg/ffprobe)

```bash
cd deepstream
python -m pytest test/test_clip_extraction_e2e.py -v
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
