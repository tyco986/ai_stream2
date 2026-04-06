"""Integration tests for Kafka → DB → Alert → WebSocket pipeline."""
import json
from unittest.mock import MagicMock, patch

import pytest
from django.utils.timezone import now

from apps.alerts.models import Alert
from apps.detections.models import Detection, KafkaDeadLetter
from services.kafka_consumer import DetectionConsumer
from tests.factories import (
    AlertRuleFactory,
    CameraFactory,
    OrganizationFactory,
)


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


def _make_msg(data, topic="deepstream-detections"):
    msg = MagicMock()
    msg.value.return_value = json.dumps(data).encode("utf-8")
    msg.topic.return_value = topic
    msg.partition.return_value = 0
    msg.offset.return_value = 1
    return msg


@pytest.mark.django_db
class TestKafkaToDbPipeline:
    def test_normal_message_creates_detection(self, consumer, camera):
        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        ts = now().isoformat()

        msg = _make_msg({
            "sensorId": camera.uid,
            "@timestamp": ts,
            "objects": [
                {"type": "person", "confidence": 0.9, "bbox": [1, 2, 3, 4]},
            ],
            "analytics": {"lineCrossing": [{"name": "gate-1", "in": 1, "out": 0}]},
        })
        det = consumer._safe_parse(msg)
        assert det is not None

        kafka_consumer_mock = MagicMock()
        with patch("services.kafka_consumer.get_channel_layer") as mock_cl:
            mock_cl.return_value = MagicMock()
            consumer._flush_detections([det])

        assert Detection.objects.count() == 1
        saved = Detection.objects.first()
        assert saved.camera == camera
        assert saved.analytics is not None

    def test_malformed_message_goes_to_dead_letter(self, consumer):
        msg = MagicMock()
        msg.value.return_value = b"{{invalid json"
        msg.topic.return_value = "deepstream-detections"
        msg.partition.return_value = 0
        msg.offset.return_value = 42

        result = consumer._safe_parse(msg)
        assert result is None
        assert KafkaDeadLetter.objects.count() == 1

    @patch("tasks.notifications.send_alert_notification.delay")
    @patch("services.kafka_consumer.get_channel_layer")
    def test_detection_triggers_alert(self, mock_cl, mock_notify, consumer, camera):
        mock_cl.return_value = MagicMock()
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_count",
            conditions={"min_count": 1},
            cooldown_seconds=0,
        )

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        ts = now().isoformat()

        msg = _make_msg({
            "sensorId": camera.uid,
            "@timestamp": ts,
            "objects": [{"type": "person", "confidence": 0.9}],
        })
        det = consumer._safe_parse(msg)
        consumer._flush_detections([det])

        assert Detection.objects.count() == 1
        assert Alert.objects.count() == 1
        alert = Alert.objects.first()
        assert alert.camera == camera
        assert alert.status == "pending"

    @patch("tasks.notifications.send_alert_notification.delay")
    @patch("services.kafka_consumer.get_channel_layer")
    def test_overcrowding_analytics_triggers_alert(self, mock_cl, mock_notify, consumer, camera):
        mock_cl.return_value = MagicMock()
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="overcrowding",
            conditions={"zone_name": "lobby"},
            cooldown_seconds=0,
        )

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        ts = now().isoformat()

        msg = _make_msg({
            "sensorId": camera.uid,
            "@timestamp": ts,
            "objects": [],
            "analytics": {
                "overcrowding": {
                    "roi_name": "lobby",
                    "triggered": True,
                    "count": 15,
                },
            },
        })
        det = consumer._safe_parse(msg)
        consumer._flush_detections([det])

        assert Alert.objects.count() == 1

    @patch("tasks.notifications.send_alert_notification.delay")
    @patch("services.kafka_consumer.get_channel_layer")
    def test_line_crossing_triggers_alert(self, mock_cl, mock_notify, consumer, camera):
        mock_cl.return_value = MagicMock()
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="line_crossing",
            conditions={"line_name": "gate-1", "min_count": 2},
            cooldown_seconds=0,
        )

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        ts = now().isoformat()

        msg = _make_msg({
            "sensorId": camera.uid,
            "@timestamp": ts,
            "objects": [],
            "analytics": {
                "lineCrossing": [{"name": "gate-1", "in": 2, "out": 1}],
            },
        })
        det = consumer._safe_parse(msg)
        consumer._flush_detections([det])

        assert Alert.objects.count() == 1

    @patch("services.kafka_consumer.get_channel_layer")
    def test_websocket_push_on_detection(self, mock_cl, consumer, camera):
        mock_layer = MagicMock()
        mock_cl.return_value = mock_layer

        consumer._camera_cache = {camera.uid: camera}
        consumer._cache_loaded_at = 9999999999
        ts = now().isoformat()

        msg = _make_msg({
            "sensorId": camera.uid,
            "@timestamp": ts,
            "objects": [{"type": "person", "confidence": 0.9}],
        })
        det = consumer._safe_parse(msg)
        consumer._flush_detections([det])

        mock_layer.group_send.assert_called()
        call_args = mock_layer.group_send.call_args_list
        group_names = [c[0][0] for c in call_args]
        assert any("detections_" in g for g in group_names)
