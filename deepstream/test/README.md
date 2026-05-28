# DeepStream API Test Guide

## Scope

- REST: health, stream add/remove, stream info
- Kafka commands: `switch_preview`, `toggle_osd`

## Files

- `test_deepstream_api.py` — integration (running container)
- `test_unit.py` — `MessageHandler` unit tests
- `conftest.py`, `_common.py` — fixtures and helpers

## Unit tests

```bash
cd deepstream
python -m pytest test/test_unit.py --noconftest -v
```

## Integration tests

```bash
pytest deepstream/test/test_deepstream_api.py -v \
  --base-url http://127.0.0.1:9000 \
  --kafka-broker 127.0.0.1:19092 \
  --camera-url rtsp://your-camera/stream
```
