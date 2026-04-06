import json
import logging
import threading

from confluent_kafka import Consumer

logger = logging.getLogger(__name__)


class CommandConsumer:
    """Daemon thread that consumes ``deepstream-commands`` Kafka topic and
    dispatches recording / screenshot / preview-switch commands.

    Command message format (JSON):
        {"action": "start_recording", "source_id": "cam_001", "duration": 20, "type": "event"}
        {"action": "start_recording", "source_id": "cam_001", "duration": 0, "type": "manual"}
        {"action": "stop_recording",  "source_id": "cam_001"}
        {"action": "screenshot",      "source_id": "cam_001", "filename": "cam001_snap.jpg"}
        {"action": "start_rolling",   "source_id": "cam_001"}
        {"action": "stop_rolling",    "source_id": "cam_001"}
        {"action": "switch_preview",  "source_id": -1}

    ``source_id`` is the **sensor_id string** for most commands (resolved to
    int via ``_resolve_source_id``).  ``switch_preview`` uses an **integer**
    directly (``-1`` = multi-view, ``N`` = single source).
    """

    def __init__(self, rolling_manager, sr_controller,
                 screenshot_retriever, tiler_element,
                 source_map, kafka_config, command_topic):
        self._rolling = rolling_manager
        self._sr = sr_controller
        self._screenshot = screenshot_retriever
        self._tiler = tiler_element
        self._source_map = source_map
        self._shutdown = threading.Event()
        self._command_topic = command_topic

        self._consumer = Consumer(kafka_config)
        self._thread = threading.Thread(
            target=self._run, daemon=True, name="cmd-consumer",
        )
        self._thread.start()

    # ------------------------------------------------------------------
    # lifecycle
    # ------------------------------------------------------------------

    def stop(self):
        self._shutdown.set()
        self._thread.join(timeout=5)

    # ------------------------------------------------------------------
    # poll loop
    # ------------------------------------------------------------------

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

    # ------------------------------------------------------------------
    # dispatch
    # ------------------------------------------------------------------

    def _dispatch(self, cmd):
        action = cmd.get("action")

        if action == "start_rolling":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._rolling.start_rolling(source_id)

        elif action == "stop_rolling":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._rolling.stop_rolling(source_id)

        elif action == "start_recording":
            source_id = self._resolve_source_id(cmd["source_id"])
            rec_type = cmd.get("type", "event")
            if rec_type == "manual":
                self._rolling.start_manual_recording(source_id)
            else:
                duration = cmd.get("duration", 20)
                self._rolling.start_event_recording(source_id, duration)

        elif action == "stop_recording":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._rolling.stop_recording(source_id)

        elif action == "screenshot":
            source_id = self._resolve_source_id(cmd["source_id"])
            self._screenshot.request_screenshot(source_id, cmd["filename"])

        elif action == "switch_preview":
            self._tiler.set_property("show-source", int(cmd["source_id"]))
            logger.info("Preview switched to source_id=%s", cmd["source_id"])

        else:
            logger.warning("Unknown command action: %s", action)

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def _resolve_source_id(self, sensor_id: str) -> int:
        """Convert sensor_id string (e.g. ``cam_001``) to the integer
        ``source_id`` assigned by nvmultiurisrcbin.  The mapping is
        maintained in the ``on_message`` callback in main.py.
        """
        source_id = self._source_map.get(sensor_id)
        if source_id is None:
            raise ValueError(f"Unknown sensor_id: {sensor_id}")
        return source_id
