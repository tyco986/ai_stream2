"""pytest configuration and shared fixtures for DeepStream integration tests.

Usage:
  # Prerequisites: DeepStream container running, RTSP test server started inside container
  pytest deepstream/test/ -v \
      --base-url http://127.0.0.1:9000 \
      --kafka-broker 127.0.0.1:19092 \
      --camera-url rtsp://127.0.0.1:8555/video1
"""

import dataclasses

import pytest

from _common import build_producer, prepare_camera, reset_streams


@dataclasses.dataclass(frozen=True)
class DSConfig:
    base_url: str
    kafka_broker: str
    command_topic: str
    camera_url: str
    camera_id: str
    camera_name: str
    timeout: int


def pytest_addoption(parser):
    parser.addoption("--base-url", default="http://127.0.0.1:9000", help="DeepStream REST base URL")
    parser.addoption("--kafka-broker", default="127.0.0.1:19092", help="Kafka bootstrap server")
    parser.addoption("--command-topic", default="deepstream-commands", help="Kafka command topic")
    parser.addoption("--camera-url", default="rtsp://127.0.0.1:8555/video1", help="Camera RTSP URL")
    parser.addoption("--camera-id", default="test_camera", help="Camera ID for tests")
    parser.addoption("--camera-name", default="Test Camera", help="Camera display name")
    parser.addoption("--timeout", type=int, default=15, help="Request timeout seconds")


@pytest.fixture(scope="session")
def ds(request) -> DSConfig:
    return DSConfig(
        base_url=request.config.getoption("--base-url"),
        kafka_broker=request.config.getoption("--kafka-broker"),
        command_topic=request.config.getoption("--command-topic"),
        camera_url=request.config.getoption("--camera-url"),
        camera_id=request.config.getoption("--camera-id"),
        camera_name=request.config.getoption("--camera-name"),
        timeout=request.config.getoption("--timeout"),
    )


@pytest.fixture(scope="session")
def prepared_camera(ds) -> int:
    """Add a test camera and return its source_id; cleanup on session end."""
    source_id = prepare_camera(ds.base_url, ds.timeout, ds.camera_id, ds.camera_name, ds.camera_url)
    yield source_id
    reset_streams(ds.base_url, ds.timeout, ds.camera_url)


@pytest.fixture(scope="session")
def kafka_producer(ds):
    return build_producer(ds.kafka_broker)
