"""Kafka consumer unit tests — message parsing, dead letter, camera status."""
import json
from unittest.mock import MagicMock, patch

import pytest
from django.utils.timezone import now

from apps.detections.models import Detection, KafkaDeadLetter
from services.kafka_consumer import DetectionConsumer
from tests.factories import CameraFactory, OrganizationFactory


@pytest.fixture
def org():
    return OrganizationFactory()


@pytest.fixture
def camera(org):
    return CameraFactory(organization=org)


@pytest.fixture
def consumer():
    with patch("services.kafka_consumer.signal"):
        return DetectionConsumer()


def _make_kafka_msg(data, topic="deepstream-detections"):
    msg = MagicMock()
    msg.value.return_value = json.dumps(data).encode("utf-8")
    msg.topic.return_value = topic
    msg.partition.return_value = 0
    msg.offset.return_value = 1
    return msg


@pytest.mark.django_db
class TestParseMessage:
    def test_valid_message_returns_detection(self, consumer, camera):
        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        ts = now().isoformat()
        msg = _make_kafka_msg({
            "sensorId": camera.uid,
            "@timestamp": ts,
            "frame_number": 42,
            "objects": [
                {"type": "person", "confidence": 0.9, "bbox": [1, 2, 3, 4]},
            ],
            "analytics": {"lineCrossing": []},
        })
        result = consumer._parse_message(msg)
        assert isinstance(result, Detection)
        assert result.camera == camera
        assert result.object_count == 1
        assert result.analytics == {"lineCrossing": []}

    def test_unknown_sensor_returns_none(self, consumer):
        consumer._camera_cache = {}
        consumer._cache_loaded_at = 9999999999
        msg = _make_kafka_msg({
            "sensorId": "nonexistent",
            "@timestamp": now().isoformat(),
            "objects": [],
        })
        result = consumer._parse_message(msg)
        assert result is None

    def test_invalid_timestamp_raises(self, consumer, camera):
        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        msg = _make_kafka_msg({
            "sensorId": camera.uid,
            "@timestamp": "not-a-date",
            "objects": [],
        })
        with pytest.raises(ValueError, match="Invalid @timestamp"):
            consumer._parse_message(msg)


@pytest.mark.django_db
class TestSafeParse:
    def test_malformed_json_creates_dead_letter(self, consumer):
        msg = MagicMock()
        msg.value.return_value = b"not json at all"
        msg.topic.return_value = "deepstream-detections"
        msg.partition.return_value = 0
        msg.offset.return_value = 99
        result = consumer._safe_parse(msg)
        assert result is None
        assert KafkaDeadLetter.objects.count() == 1
        dl = KafkaDeadLetter.objects.first()
        assert dl.offset == 99
        assert "json" in dl.error_message.lower() or "Expecting" in dl.error_message

    def test_missing_sensor_id_creates_dead_letter(self, consumer):
        msg = _make_kafka_msg({"no_sensor": True, "@timestamp": now().isoformat()})
        result = consumer._safe_parse(msg)
        assert result is None
        assert KafkaDeadLetter.objects.count() == 1


@pytest.mark.django_db
class TestCameraStatusFromKafka:
    def test_stream_started_updates_status(self, consumer, camera):
        camera.status = "connecting"
        camera.save()

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999

        with patch("services.kafka_consumer.get_channel_layer") as mock_cl:
            mock_cl.return_value = MagicMock()
            consumer._update_camera_status({
                "event": "camera_online",
                "sensorId": camera.uid,
            })

        camera.refresh_from_db()
        assert camera.status == "online"

    def test_stream_error_updates_status(self, consumer, camera):
        camera.status = "online"
        camera.save()

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999

        with patch("services.kafka_consumer.get_channel_layer") as mock_cl:
            mock_cl.return_value = MagicMock()
            consumer._update_camera_status({
                "event": "camera_error",
                "sensorId": camera.uid,
            })

        camera.refresh_from_db()
        assert camera.status == "error"

    def test_unknown_event_ignored(self, consumer, camera):
        camera.status = "online"
        camera.save()

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999

        consumer._update_camera_status({
            "event": "recording_done",
            "sensorId": camera.uid,
        })

        camera.refresh_from_db()
        assert camera.status == "online"
