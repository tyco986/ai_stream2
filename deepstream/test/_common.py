#!/usr/bin/env python3
"""Common helpers for DeepStream black-box API tests."""

import argparse
import json
import os
import time
from pathlib import Path
from typing import Any

import requests
from confluent_kafka import Producer


DEEPSTREAM_DIR = Path(__file__).resolve().parents[1]
EXAMPLE_DATA_DIR = DEEPSTREAM_DIR / "example_data"
TEST_DATA_DIR = EXAMPLE_DATA_DIR / "test_data"


class DeliveryReporter:
    """Track Kafka delivery result without nested callback functions."""

    def __init__(self):
        self.delivered = 0
        self.last_error = None

    def on_delivery(self, err, msg):
        if err is not None:
            self.last_error = str(err)
            return
        self.delivered += 1


def create_parser(description: str, include_kafka: bool = False) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--base-url", default="http://127.0.0.1:9000", help="DeepStream REST base URL")
    parser.add_argument("--timeout", type=int, default=15, help="Request timeout seconds")
    parser.add_argument("--camera-id", default=generate_camera_id(), help="Camera ID used in tests")
    parser.add_argument(
        "--camera-url",
        default="file:///app/example_data/video2_bf0.mp4",
        help="Camera stream URL used for stream/add",
    )
    parser.add_argument("--camera-name", default="API Test Camera", help="Camera display name")
    parser.add_argument("--verbose", action="store_true", help="Print response payloads")
    if include_kafka:
        parser.add_argument("--kafka-broker", default="127.0.0.1:9092", help="Kafka bootstrap server")
        parser.add_argument("--command-topic", default="deepstream-commands", help="Kafka command topic")
        parser.add_argument(
            "--event-topic",
            default="deepstream-events",
            help="Kafka events topic (reserved for future validation)",
        )
    return parser


def generate_camera_id() -> str:
    suffix = int(time.time() * 1000) % 100000000
    return f"cam_test_{suffix}"


def ensure_test_data_dir() -> Path:
    TEST_DATA_DIR.mkdir(parents=True, exist_ok=True)
    return TEST_DATA_DIR


def write_test_payload(name: str, payload: dict[str, Any]) -> Path:
    target = ensure_test_data_dir() / name
    target.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return target


def build_stream_add_payload(camera_id: str, camera_name: str, camera_url: str) -> dict[str, Any]:
    return {
        "key": "sensor",
        "value": {
            "camera_id": camera_id,
            "camera_name": camera_name,
            "camera_url": camera_url,
            "change": "camera_add",
        },
    }


def build_stream_remove_payload(camera_id: str, camera_url: str) -> dict[str, Any]:
    return {
        "key": "sensor",
        "value": {
            "camera_id": camera_id,
            "camera_url": camera_url,
            "change": "camera_remove",
        },
    }


def build_rest_url(base_url: str, endpoint: str) -> str:
    return f"{base_url.rstrip('/')}{endpoint}"


def http_get_json(base_url: str, endpoint: str, timeout: int) -> tuple[requests.Response, Any]:
    url = build_rest_url(base_url, endpoint)
    response = _request_with_retry("GET", url, timeout=timeout)
    return response, _parse_json(response)


def http_post_json(base_url: str, endpoint: str, payload: dict[str, Any], timeout: int) -> tuple[requests.Response, Any]:
    url = build_rest_url(base_url, endpoint)
    response = _request_with_retry("POST", url, json=payload, timeout=timeout)
    return response, _parse_json(response)


def _request_with_retry(method: str, url: str, timeout: int, json: dict[str, Any] | None = None) -> requests.Response:
    session = requests.Session()
    # Controlled retries for transient connection resets/timeouts from DS REST.
    attempts = 4
    for idx in range(attempts):
        try:
            return session.request(method=method, url=url, json=json, timeout=timeout)
        except requests.exceptions.RequestException:
            if idx == attempts - 1:
                raise
            time.sleep(1 + idx)
    raise RuntimeError("request retry loop should not reach here")


def _parse_json(response: requests.Response) -> Any:
    if not response.text.strip():
        return {}
    return response.json()


def assert_status(response: requests.Response, expected_codes: set[int], context: str):
    if response.status_code in expected_codes:
        return
    raise AssertionError(
        f"{context} expected status in {sorted(expected_codes)}, got {response.status_code}, body={response.text}"
    )


def verbose_print(enabled: bool, title: str, data: Any):
    if enabled:
        print(f"[DEBUG] {title}: {json.dumps(data, ensure_ascii=False)}")


def build_producer(kafka_broker: str) -> Producer:
    return Producer({"bootstrap.servers": kafka_broker})


def send_command(producer: Producer, topic: str, payload: dict[str, Any], timeout: int):
    reporter = DeliveryReporter()
    producer.produce(topic, json.dumps(payload).encode("utf-8"), callback=reporter.on_delivery)
    pending = producer.flush(timeout=timeout)
    if reporter.last_error:
        raise AssertionError(f"Kafka command delivery failed: {reporter.last_error}")
    if reporter.delivered != 1 or pending != 0:
        raise AssertionError(
            f"Kafka command not fully delivered: delivered={reporter.delivered}, pending={pending}"
        )


def fetch_stream_info(base_url: str, timeout: int) -> tuple[requests.Response, Any]:
    return http_get_json(base_url, "/api/v1/stream/get-stream-info", timeout)


def get_stream_entries(stream_info_payload: Any) -> list[dict[str, Any]]:
    if not isinstance(stream_info_payload, dict):
        return []
    stream_info = stream_info_payload.get("stream-info")
    if not isinstance(stream_info, dict):
        return []
    entries = stream_info.get("stream-info")
    if not isinstance(entries, list):
        return []
    return [entry for entry in entries if isinstance(entry, dict)]


def reset_streams(base_url: str, timeout: int, camera_url: str):
    response, data = fetch_stream_info(base_url, timeout)
    assert_status(response, {200}, "stream/get-stream-info for reset")
    entries = get_stream_entries(data)
    for entry in entries:
        camera_id = first_existing_value(entry, ["camera_id", "cameraId", "sensor_id", "sensorId", "id"])
        if camera_id is None:
            continue
        remove_payload = build_stream_remove_payload(str(camera_id), camera_url)
        remove_response, _ = http_post_json(base_url, "/api/v1/stream/remove", remove_payload, timeout)
        assert_status(remove_response, {200, 201, 404}, "stream/remove during reset")


def ensure_camera_added(base_url: str, timeout: int, camera_id: str, camera_name: str, camera_url: str):
    payload = build_stream_add_payload(camera_id, camera_name, camera_url)
    response, data = http_post_json(base_url, "/api/v1/stream/add", payload, timeout)
    assert_status(response, {200, 201}, "stream/add")
    verbose_print(False, "stream/add response", data)


def ensure_camera_removed(base_url: str, timeout: int, camera_id: str, camera_url: str):
    payload = build_stream_remove_payload(camera_id, camera_url)
    response, data = http_post_json(base_url, "/api/v1/stream/remove", payload, timeout)
    assert_status(response, {200, 201, 404}, "stream/remove")
    verbose_print(False, "stream/remove response", data)


def flatten_dict_candidates(obj: Any) -> list[dict[str, Any]]:
    if isinstance(obj, dict):
        items = [obj]
        for value in obj.values():
            items.extend(flatten_dict_candidates(value))
        return items
    if isinstance(obj, list):
        items = []
        for value in obj:
            items.extend(flatten_dict_candidates(value))
        return items
    return []


def find_source_id_by_camera_id(stream_info: Any, camera_id: str) -> int | None:
    candidates = flatten_dict_candidates(stream_info)
    camera_keys = ["camera_id", "cameraId", "sensor_id", "sensorId", "id"]
    source_keys = ["source_id", "sourceId", "source-index", "source_index"]
    for item in candidates:
        camera_value = first_existing_value(item, camera_keys)
        if str(camera_value) != camera_id:
            continue
        source_value = first_existing_value(item, source_keys)
        if source_value is None:
            continue
        if isinstance(source_value, int):
            return source_value
        if isinstance(source_value, str) and source_value.isdigit():
            return int(source_value)
    return None


def camera_exists(stream_info: Any, camera_id: str) -> bool:
    candidates = flatten_dict_candidates(stream_info)
    camera_keys = ["camera_id", "cameraId", "sensor_id", "sensorId", "id"]
    for item in candidates:
        camera_value = first_existing_value(item, camera_keys)
        if str(camera_value) == camera_id:
            return True
    return False


def first_existing_value(data: dict[str, Any], keys: list[str]) -> Any:
    for key in keys:
        if key in data:
            return data[key]
    return None


def wait_for_source_id(base_url: str, timeout: int, camera_id: str, wait_seconds: int = 60) -> int:
    started = time.time()
    while time.time() - started < wait_seconds:
        _, stream_info = fetch_stream_info(base_url, timeout)
        source_id = find_source_id_by_camera_id(stream_info, camera_id)
        if source_id is not None:
            return source_id
        time.sleep(1)
    raise AssertionError(f"Timed out waiting for source_id mapped from camera_id={camera_id}")


def prepare_camera(base_url: str, timeout: int, camera_id: str, camera_name: str, camera_url: str) -> int:
    reset_streams(base_url, timeout, camera_url)
    ensure_camera_added(base_url, timeout, camera_id, camera_name, camera_url)
    return wait_for_source_id(base_url, timeout, camera_id)


def parse_source_id(stream_info_payload: Any, camera_id: str) -> int:
    source_id = find_source_id_by_camera_id(stream_info_payload, camera_id)
    if source_id is None:
        raise AssertionError(f"source_id not found for camera_id={camera_id}")
    return source_id


def wait_for_file(path: Path, wait_seconds: int = 20) -> bool:
    started = time.time()
    while time.time() - started < wait_seconds:
        if path.exists() and path.is_file() and path.stat().st_size > 0:
            return True
        time.sleep(1)
    return False


def get_screenshots_dir() -> Path:
    env_value = os.environ.get("DS_SCREENSHOTS_DIR")
    if env_value:
        return Path(env_value)
    return DEEPSTREAM_DIR / "screenshots"


def get_recordings_dirs() -> tuple[Path, Path]:
    rolling = Path(os.environ.get("DS_ROLLING_DIR", str(DEEPSTREAM_DIR / "recordings" / "rolling")))
    locked = Path(os.environ.get("DS_LOCKED_DIR", str(DEEPSTREAM_DIR / "recordings" / "locked")))
    return rolling, locked


def count_files(path: Path) -> int:
    if not path.exists() or not path.is_dir():
        return 0
    return sum(1 for item in path.iterdir() if item.is_file())

