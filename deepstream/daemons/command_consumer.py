import json
import logging
import threading

from confluent_kafka import Consumer, Producer

logger = logging.getLogger(__name__)


class CommandConsumer:
    """Daemon thread that consumes ``deepstream-commands`` Kafka topic and
    dispatches preview-switch commands.

    Command message format (JSON):
        {"action": "switch_preview",  "source_id": -1}
        {"action": "toggle_osd",      "show": true}

    ``switch_preview`` uses an **integer** ``source_id`` directly
    (``-1`` = multi-view, ``N`` = single source).
    """

    def __init__(self, tiler_element, osd_toggle,
                 kafka_config, command_topic, event_topic: str | None = None):
        self._tiler = tiler_element
        self._osd_toggle = osd_toggle
        self._shutdown = threading.Event()
        self._command_topic = command_topic
        self._event_topic = event_topic or "deepstream-events"
        bootstrap_servers = kafka_config.get("bootstrap.servers", "kafka:9092")
        self._producer = Producer({"bootstrap.servers": bootstrap_servers})

        self._consumer = Consumer(kafka_config)

        self._thread = threading.Thread(
            target=self._run, daemon=True, name="cmd-consumer",
        )
        self._thread.start()

    def stop(self):
        self._shutdown.set()
        self._thread.join(timeout=5)
        self._producer.flush(timeout=5)

    def _run(self):
        self._consumer.subscribe([self._command_topic])
        logger.info("CommandConsumer subscribed to %s", self._command_topic)

        while not self._shutdown.is_set():
            msg = self._consumer.poll(timeout=1.0)
            if msg is None:
                continue
            if msg.error():
                logger.warning("Kafka consumer error: %s", msg.error())
                continue
            try:
                cmd = json.loads(msg.value())
                self._dispatch(cmd)
            except Exception:
                logger.exception(
                    "Failed to process command: %s",
                    msg.value()[:500] if msg.value() else "<empty>",
                )

        self._consumer.close()
        logger.info("CommandConsumer stopped")

    def _dispatch(self, cmd):
        action = cmd.get("action")

        if action == "switch_preview":
            if hasattr(self._tiler, "set"):
                self._tiler.set({"show-source": int(cmd["source_id"])})
                logger.info("Preview switched to source_id=%s", cmd["source_id"])
            else:
                self._publish_command_error(
                    action=action,
                    source_id=cmd["source_id"],
                    reason="set not supported by tiler node",
                )
                raise RuntimeError("set not supported by tiler node")

        elif action == "toggle_osd":
            show = cmd.get("show", True)
            self._osd_toggle.set_overlay(bool(show))

        else:
            logger.warning("Unknown command action: %s", action)

    def _publish_command_error(self, action: str, source_id, reason: str):
        event = {
            "event": "command_error",
            "action": action,
            "source_id": source_id,
            "reason": reason,
        }
        self._producer.produce(
            self._event_topic,
            value=json.dumps(event).encode("utf-8"),
        )
        self._producer.poll(0)
