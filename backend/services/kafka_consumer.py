import json
import signal
import time

import structlog
from asgiref.sync import async_to_sync
from channels.layers import get_channel_layer
from confluent_kafka import Consumer, KafkaError
from django.conf import settings
from django.db import transaction
from django.utils.dateparse import parse_datetime

from apps.detections.models import Detection, KafkaDeadLetter
from services.alert_engine import AlertEngine

logger = structlog.get_logger(__name__)


class DetectionConsumer:
    MAX_CONSECUTIVE_FAILURES = 5

    def __init__(self):
        self._shutdown = False
        self._camera_cache = {}
        self._cache_ttl = 300
        self._cache_loaded_at = 0
        self._alert_engine = AlertEngine()
        self._consecutive_failures = 0
        self._flush_count = 0
        self._batch_size = getattr(settings, "KAFKA_BATCH_SIZE", 100)
        self._flush_interval = getattr(settings, "KAFKA_FLUSH_INTERVAL", 2.0)
        signal.signal(signal.SIGTERM, self._handle_signal)
        signal.signal(signal.SIGINT, self._handle_signal)

    def _handle_signal(self, signum, frame):
        logger.info("Received signal, shutting down gracefully...", signal=signum)
        self._shutdown = True

    def _kafka_config(self):
        return {
            "bootstrap.servers": settings.KAFKA_BOOTSTRAP_SERVERS,
            "group.id": settings.KAFKA_CONSUMER_GROUP,
            "auto.offset.reset": "latest",
            "enable.auto.commit": False,
        }

    def _get_camera(self, sensor_id):
        """内存缓存 sensorId → Camera 映射，避免每条消息查库。"""
        current = time.time()
        if current - self._cache_loaded_at > self._cache_ttl:
            from apps.cameras.models import Camera  # noqa: E402 — 解决循环依赖
            self._camera_cache = {
                c.uid: c for c in Camera.objects.filter(is_deleted=False).select_related("organization")
            }
            self._cache_loaded_at = current
        return self._camera_cache.get(sensor_id)

    def run(self):
        consumer = Consumer(self._kafka_config())
        topics = [settings.KAFKA_DETECTION_TOPIC, settings.KAFKA_EVENT_TOPIC]
        consumer.subscribe(topics)
        logger.info("Kafka consumer subscribed", topics=topics)

        detection_buffer = []
        last_flush = time.time()

        while not self._shutdown:
            msg = consumer.poll(timeout=1.0)
            if msg is not None:
                if msg.error():
                    if msg.error().code() != KafkaError._PARTITION_EOF:
                        logger.warning("Kafka consumer error", error=str(msg.error()))
                    continue

                topic = msg.topic()
                if topic == settings.KAFKA_EVENT_TOPIC:
                    self._handle_event(msg)
                    continue

                parsed = self._safe_parse(msg)
                if parsed is not None:
                    detection_buffer.append(parsed)

            should_flush = (
                len(detection_buffer) >= self._batch_size
                or (detection_buffer and time.time() - last_flush >= self._flush_interval)
            )
            if should_flush:
                self._try_flush(detection_buffer, consumer)
                detection_buffer.clear()
                last_flush = time.time()

        if detection_buffer:
            logger.info("Flushing remaining detections before shutdown",
                        count=len(detection_buffer))
            self._try_flush(detection_buffer, consumer)
        consumer.close()
        logger.info("Kafka consumer closed cleanly")

    def _try_flush(self, buffer, consumer):
        try:
            self._flush_detections(buffer)
            consumer.commit(asynchronous=False)
            self._consecutive_failures = 0
        except Exception:
            self._consecutive_failures += 1
            logger.exception("Flush failed",
                             consecutive_failures=self._consecutive_failures)
            if self._consecutive_failures >= self.MAX_CONSECUTIVE_FAILURES:
                logger.critical("Too many consecutive flush failures, crashing for restart")
                raise

    def _safe_parse(self, msg):
        """解析失败写死信，不中断消费循环。"""
        raw = msg.value()
        try:
            return self._parse_message(msg)
        except (json.JSONDecodeError, KeyError, ValueError) as e:
            KafkaDeadLetter.objects.create(
                topic=msg.topic(),
                partition_num=msg.partition(),
                offset=msg.offset(),
                raw_message=raw.decode("utf-8", errors="replace")[:10000],
                error_message=str(e),
            )
            logger.warning("Message sent to dead letter",
                           topic=msg.topic(), offset=msg.offset(), error=str(e))
            return None

    def _parse_message(self, msg):
        raw = msg.value()
        data = json.loads(raw.decode("utf-8"))
        camera = self._get_camera(data["sensorId"])
        if camera is None:
            return None
        detected_at = parse_datetime(data["@timestamp"])
        if detected_at is None:
            raise ValueError(f"Invalid @timestamp: {data.get('@timestamp')}")
        return Detection(
            camera=camera,
            detected_at=detected_at,
            frame_number=data.get("frame_number"),
            object_count=len(data.get("objects", [])),
            detected_objects=data.get("objects", []),
            analytics=data.get("analytics"),
        )

    def _flush_detections(self, buffer):
        valid = [d for d in buffer if d is not None]
        if not valid:
            return

        with transaction.atomic():
            Detection.objects.bulk_create(valid)

        self._flush_count += 1
        if self._flush_count % 50 == 0:
            self._alert_engine.prune_cooldown_cache()

        alerts_to_create = []
        for detection in valid:
            triggered = self._alert_engine.evaluate_detection(detection)
            alerts_to_create.extend(triggered)

        if alerts_to_create:
            from apps.alerts.models import Alert  # noqa: E402 — 解决循环依赖
            created_alerts = Alert.objects.bulk_create(alerts_to_create)
            self._push_alerts(created_alerts)
            self._schedule_notifications(created_alerts)

        self._push_detections(valid)

    def _push_detections(self, detections):
        """Best-effort WebSocket push — Redis failure does not block flush."""
        try:
            channel_layer = get_channel_layer()
            by_org = {}
            for d in detections:
                org_id = str(d.camera.organization_id)
                by_org.setdefault(org_id, []).append({
                    "camera_id": str(d.camera_id),
                    "camera_uid": d.camera.uid,
                    "object_count": d.object_count,
                    "detected_at": d.detected_at.isoformat(),
                })
            for org_id, items in by_org.items():
                async_to_sync(channel_layer.group_send)(
                    f"detections_{org_id}",
                    {"type": "detection.new", "data": items},
                )
        except Exception:
            logger.warning("WebSocket push failed (best-effort)", exc_info=True)

    def _push_alerts(self, alerts):
        """Best-effort WebSocket push for alerts."""
        try:
            channel_layer = get_channel_layer()
            for alert in alerts:
                async_to_sync(channel_layer.group_send)(
                    f"alerts_{alert.organization_id}",
                    {
                        "type": "alert.new",
                        "data": {
                            "alert_id": str(alert.id),
                            "rule_name": alert.rule.name,
                            "camera_uid": alert.camera.uid,
                            "triggered_at": alert.triggered_at.isoformat(),
                        },
                    },
                )
        except Exception:
            logger.warning("Alert WebSocket push failed (best-effort)", exc_info=True)

    def _schedule_notifications(self, alerts):
        """Dispatch Celery tasks to send alert notifications (email/webhook)."""
        from tasks.notifications import send_alert_notification  # noqa: E402 — 解决循环依赖
        for alert in alerts:
            if alert.rule.notify_channels:
                send_alert_notification.delay(str(alert.id))

    def _handle_event(self, msg):
        """Handle events from deepstream-events topic.

        Updates Camera.status based on stream lifecycle events
        (camera_online, camera_offline, camera_error, etc.)
        and pushes status changes via WebSocket.
        """
        try:
            data = json.loads(msg.value().decode("utf-8"))
            event_type = data.get("event")
            logger.info("DeepStream event received", event_type=event_type, data=data)
            self._update_camera_status(data)
        except Exception:
            logger.warning("Failed to parse DeepStream event", exc_info=True)

    def _update_camera_status(self, data):
        """Map DeepStream event to Camera.status and persist + push."""
        from apps.cameras.models import Camera  # noqa: E402 — 解决循环依赖

        event_type = data.get("event")
        sensor_id = data.get("sensorId") or data.get("camera_id")
        if not sensor_id or not event_type:
            return

        status_map = {
            "camera_online": Camera.Status.ONLINE,
            "stream_started": Camera.Status.ONLINE,
            "camera_offline": Camera.Status.OFFLINE,
            "stream_removed": Camera.Status.OFFLINE,
            "camera_error": Camera.Status.ERROR,
            "stream_error": Camera.Status.ERROR,
        }
        new_status = status_map.get(event_type)
        if not new_status:
            return

        camera = self._camera_cache.get(sensor_id)
        if not camera:
            camera = Camera.all_objects.filter(uid=sensor_id).select_related("organization").first()
        if not camera or camera.status == new_status:
            return

        old_status = camera.status
        camera.status = new_status
        camera.save(update_fields=["status", "updated_at"])
        logger.info("Camera status updated from Kafka event",
                     camera_uid=sensor_id, old_status=old_status, new_status=new_status)

        try:
            channel_layer = get_channel_layer()
            async_to_sync(channel_layer.group_send)(
                f"camera_status_{camera.organization_id}",
                {
                    "type": "camera.status",
                    "data": {
                        "camera_id": str(camera.id),
                        "camera_uid": camera.uid,
                        "status": new_status,
                        "previous_status": old_status,
                    },
                },
            )
        except Exception:
            logger.warning("Camera status WebSocket push failed", exc_info=True)
