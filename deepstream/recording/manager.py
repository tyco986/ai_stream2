import logging
from pathlib import Path

from recording.smartrecord import SmartRecordController

logger = logging.getLogger(__name__)


class RollingRecordManager:
    """Manage rolling (7x24) recordings via SmartRecord.

    Rolling segments stay in ``rolling/`` and are cleaned by DiskGuard.
    """

    SEGMENT_DURATION = 300  # seconds (5 min)

    def __init__(self, rolling_dir, segment_duration=None,
                 source_element=None):
        self._rolling_dir = Path(rolling_dir)
        self._rolling_sources = {}   # source_id → uri
        self._uri_map = {}           # source_id → uri (persists after stop_rolling)
        self._segment_duration = segment_duration or self.SEGMENT_DURATION

        self._sr_controller = SmartRecordController(
            source_element,
            on_recording_done=self._on_sr_done,
        )
        logger.info("Recording backend: smartrecord (C extension)")

        self._rolling_dir.mkdir(parents=True, exist_ok=True)

    # ------------------------------------------------------------------
    # rolling (7x24)
    # ------------------------------------------------------------------

    def register_source(self, source_id: int, uri: str):
        """Store the URI for a source so Kafka commands can restart recording."""
        self._uri_map[source_id] = uri
        self._sr_controller.register_source(source_id)

    def unregister_source(self, source_id: int):
        self._uri_map.pop(source_id, None)
        self._sr_controller.unregister_source(source_id)

    def start_rolling(self, source_id: int, uri: str = ""):
        resolved_uri = uri or self._uri_map.get(source_id, "")
        if not resolved_uri:
            logger.warning("Cannot start rolling: no URI for source_id=%d", source_id)
            return
        self._uri_map[source_id] = resolved_uri
        self._rolling_sources[source_id] = resolved_uri

        self._sr_controller.register_source(source_id)
        self._sr_controller.start(
            source_id, start_time=0, duration=self._segment_duration,
        )
        logger.info("Rolling recording started for source_id=%d uri=%s", source_id, resolved_uri)

    def stop_rolling(self, source_id: int):
        self._rolling_sources.pop(source_id, None)
        self._sr_controller.stop(source_id)
        logger.info("Rolling recording stopped for source_id=%d", source_id)

    # ------------------------------------------------------------------
    # sr-done callback
    # ------------------------------------------------------------------

    def _on_sr_done(self, source_id: int, info: dict):
        """Called by SmartRecordController when a recording segment finishes."""
        filename = info.get("filename", "")
        dirpath = info.get("dirpath", "")
        logger.info(
            "SmartRecord segment done: source_id=%d file=%s/%s",
            source_id, dirpath, filename,
        )

        if source_id in self._rolling_sources:
            self._sr_controller.start(
                source_id, start_time=0, duration=self._segment_duration,
            )

    # ------------------------------------------------------------------
    # shutdown
    # ------------------------------------------------------------------

    def shutdown(self):
        self._sr_controller.stop_all()
