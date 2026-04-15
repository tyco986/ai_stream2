import json
import logging
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime

from confluent_kafka import Consumer, Producer

from recording.clip_extractor import ClipExtractionError, RollingClipExtractor, parse_utc_iso
from utils.storage import StorageManager

logger = logging.getLogger(__name__)

_PENDING_TTL_SEC = 86400
_CLIP_WORKERS = 2


class CommandConsumer:
    """Daemon thread that consumes ``deepstream-commands`` Kafka topic and
    dispatches recording / screenshot / preview-switch commands.

    Command message format (JSON):
        {"action": "start_rolling",   "source_id": "cam_001"}
        {"action": "stop_rolling",    "source_id": "cam_001"}
        {"action": "start_recording", "source_id": "cam_001", "request_id": "<uuid>",
         "start_ts": "2026-01-01T12:00:00Z"}
        {"action": "stop_recording",  "source_id": "cam_001", "request_id": "<uuid>",
         "end_ts": "2026-01-01T12:05:00Z"}
        {"action": "screenshot",      "source_id": "cam_001", "filename": "cam001_snap.jpg"}
        {"action": "switch_preview",  "source_id": -1}
        {"action": "toggle_osd",      "show": true}

    ``start_recording`` / ``stop_recording`` register a UTC wall-clock window and
    trigger asynchronous extraction from existing ``rolling/`` MP4s into ``locked/``
    (not a second SmartRecord session).

    ``source_id`` is the **sensor_id string** for most commands (resolved to
    int via ``_resolve_source_id``).  ``switch_preview`` uses an **integer**
    directly (``-1`` = multi-view, ``N`` = single source).
    ``toggle_osd`` controls whether the preview stream includes AI overlays.
    """

    def __init__(self, rolling_manager,
                 screenshot_retriever, tiler_element,
                 osd_toggle, source_map, kafka_config, command_topic,
                 storage: StorageManager,
                 event_topic: str | None = None):
        self._rolling = rolling_manager
        self._screenshot = screenshot_retriever
        self._tiler = tiler_element
        self._osd_toggle = osd_toggle
        self._source_map = source_map
        self._storage = storage
        self._clip_extractor = RollingClipExtractor(storage)
        self._shutdown = threading.Event()
        self._command_topic = command_topic
        self._event_topic = event_topic or "deepstream-events"
        bootstrap_servers = kafka_config.get("bootstrap.servers", "kafka:9092")
        self._producer = Producer({"bootstrap.servers": bootstrap_servers})

        self._consumer = Consumer(kafka_config)
        self._pending_lock = threading.Lock()
        self._pending: dict[str, dict] = {}
        self._executor = ThreadPoolExecutor(
            max_workers=_CLIP_WORKERS,
            thread_name_prefix="clip-extract",
        )

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
        self._executor.shutdown(wait=False, cancel_futures=False)
        self._producer.flush(timeout=5)

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
            self._handle_start_recording(cmd)

        elif action == "stop_recording":
            self._handle_stop_recording(cmd)

        elif action == "screenshot":
            camera_id = self._resolve_camera_id(cmd["source_id"])
            source_id = self._resolve_source_id(cmd["source_id"])
            if hasattr(self._screenshot, "request_screenshot"):
                self._screenshot.request_screenshot(source_id, camera_id, cmd["filename"])
            else:
                self._publish_command_error(
                    action=action,
                    source_id=cmd["source_id"],
                    reason="request_screenshot not supported by screenshot handler",
                )
                raise RuntimeError("request_screenshot not supported by screenshot handler")

        elif action == "switch_preview":
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

    def _handle_start_recording(self, cmd):
        try:
            request_id = str(cmd["request_id"])
            start_ts = parse_utc_iso(str(cmd["start_ts"]))
        except KeyError as e:
            logger.warning("start_recording missing field: %s", e)
            return
        except ValueError as e:
            logger.warning("start_recording invalid timestamp: %s", e)
            return

        try:
            camera_id = self._resolve_camera_id(cmd["source_id"])
        except ValueError as e:
            self._publish_clip_failed(cmd.get("request_id", ""), "", str(e))
            return

        now = time.time()
        with self._pending_lock:
            self._prune_pending_unlocked(now)
            if request_id in self._pending:
                self._publish_clip_failed(request_id, camera_id, "duplicate request_id for start_recording")
                return
            self._pending[request_id] = {
                "camera_id": camera_id,
                "start_ts": start_ts,
                "registered_at": now,
            }
        logger.info(
            "start_recording registered request_id=%s camera_id=%s start_ts=%s",
            request_id, camera_id, start_ts.isoformat(),
        )

    def _handle_stop_recording(self, cmd):
        try:
            request_id = str(cmd["request_id"])
            end_ts = parse_utc_iso(str(cmd["end_ts"]))
        except KeyError as e:
            logger.warning("stop_recording missing field: %s", e)
            return
        except ValueError as e:
            logger.warning("stop_recording invalid timestamp: %s", e)
            return

        try:
            camera_id = self._resolve_camera_id(cmd["source_id"])
        except ValueError as e:
            self._publish_clip_failed(request_id, "", str(e))
            return

        with self._pending_lock:
            self._prune_pending_unlocked(time.time())
            pending = self._pending.pop(request_id, None)

        if pending is None:
            logger.warning(
                "stop_recording without matching start: request_id=%s source_id=%s",
                request_id, cmd.get("source_id"),
            )
            self._publish_clip_failed(
                request_id, camera_id, "no matching start_recording for this request_id",
            )
            return

        if pending["camera_id"] != camera_id:
            self._publish_clip_failed(
                request_id, camera_id, "source_id does not match pending start_recording",
            )
            return

        start_ts: datetime = pending["start_ts"]
        if end_ts <= start_ts:
            self._publish_clip_failed(request_id, camera_id, "end_ts must be after start_ts")
            return

        self._executor.submit(
            self._run_clip_job,
            camera_id,
            start_ts,
            end_ts,
            request_id,
        )

    def _prune_pending_unlocked(self, now: float):
        expired = [
            rid for rid, meta in self._pending.items()
            if now - meta["registered_at"] > _PENDING_TTL_SEC
        ]
        for rid in expired:
            meta = self._pending.pop(rid, None)
            logger.warning(
                "pending start_recording expired request_id=%s camera_id=%s",
                rid, meta.get("camera_id") if meta else "?",
            )

    def _run_clip_job(self, camera_id: str, start_ts: datetime, end_ts: datetime, request_id: str):
        try:
            path = self._clip_extractor.extract(camera_id, start_ts, end_ts, request_id)
            rel = path.relative_to(self._storage.base_dir)
            self._publish_clip_ready(request_id, camera_id, str(rel))
        except ClipExtractionError as e:
            logger.warning("Clip extraction failed: %s", e)
            self._publish_clip_failed(request_id, camera_id, str(e))
        except Exception:
            logger.exception("Clip extraction unexpected error")
            self._publish_clip_failed(request_id, camera_id, "internal error during clip extraction")

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def _resolve_source_id(self, source_ref) -> int:
        """Resolve command ``source_id`` from either sensor_id or integer.

        Accepted formats:
        - sensor_id string: "cam_001"
        - integer source_id: 3
        - numeric string source_id: "3"
        """
        if isinstance(source_ref, int):
            if source_ref in self._source_map.values():
                return source_ref
            raise ValueError(f"Unknown source_id: {source_ref}")

        if isinstance(source_ref, str) and source_ref.isdigit():
            source_id = int(source_ref)
            if source_id in self._source_map.values():
                return source_id
            raise ValueError(f"Unknown source_id: {source_ref}")

        source_id = self._source_map.get(source_ref)
        if source_id is None:
            raise ValueError(f"Unknown sensor_id: {source_ref}")
        return source_id

    def _resolve_camera_id(self, source_ref) -> str:
        """Resolve the camera_id (sensor_id string) from a command source_ref.

        If source_ref is already a sensor_id string, return it directly.
        If it is an integer or numeric string, reverse-lookup from source_map.
        """
        if isinstance(source_ref, str) and not source_ref.isdigit():
            if source_ref in self._source_map:
                return source_ref
            raise ValueError(f"Unknown sensor_id: {source_ref}")

        int_id = int(source_ref)
        for sensor_id, src_id in self._source_map.items():
            if src_id == int_id:
                return sensor_id
        raise ValueError(f"Cannot resolve camera_id for source_id: {source_ref}")

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

    def _publish_clip_ready(self, request_id: str, sensor_id: str, clip_path: str):
        event = {
            "event": "clip_ready",
            "request_id": request_id,
            "sensorId": sensor_id,
            "clip_path": clip_path,
        }
        self._producer.produce(
            self._event_topic,
            value=json.dumps(event).encode("utf-8"),
        )
        self._producer.poll(0)
        logger.info("Published clip_ready request_id=%s path=%s", request_id, clip_path)

    def _publish_clip_failed(self, request_id: str, sensor_id: str, reason: str):
        event = {
            "event": "clip_failed",
            "request_id": request_id,
            "sensorId": sensor_id,
            "reason": reason,
        }
        self._producer.produce(
            self._event_topic,
            value=json.dumps(event).encode("utf-8"),
        )
        self._producer.poll(0)
        logger.info("Published clip_failed request_id=%s reason=%s", request_id, reason)
