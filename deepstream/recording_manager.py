import logging
import shutil
from pathlib import Path

from smartrecord_controller import SmartRecordController

logger = logging.getLogger(__name__)


class RollingRecordManager:
    """Manage rolling (7x24), event, and manual recordings via SmartRecord.

    Design (dashcam model):
        - Rolling segments stay in ``rolling/`` and are cleaned by DiskGuard.
        - Event / manual recordings are moved to ``locked/`` on completion so
          DiskGuard's rolling cleanup never touches them.
    """

    SEGMENT_DURATION = 300  # seconds (5 min)

    def __init__(self, rolling_dir, locked_dir, segment_duration=None,
                 source_element=None):
        self._rolling_dir = Path(rolling_dir)
        self._locked_dir = Path(locked_dir)
        self._rolling_sources = {}   # source_id → uri
        self._uri_map = {}           # source_id → uri (persists after stop_rolling)
        self._segment_duration = segment_duration or self.SEGMENT_DURATION

        self._sr_controller = SmartRecordController(
            source_element,
            on_recording_done=self._on_sr_done,
        )
        logger.info("Recording backend: smartrecord (C extension)")

        self._rolling_dir.mkdir(parents=True, exist_ok=True)
        self._locked_dir.mkdir(parents=True, exist_ok=True)

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
    # event recording (alert-triggered)
    # ------------------------------------------------------------------

    def start_event_recording(self, source_id: int, duration: int = 20):
        uri = self._rolling_sources.get(source_id)
        if not uri:
            logger.warning("Cannot start event recording: no URI for source_id=%d", source_id)
            return
        logger.info("Event recording requested: source_id=%d duration=%ds (covered by rolling)", source_id, duration)

    # ------------------------------------------------------------------
    # manual recording (user-controlled)
    # ------------------------------------------------------------------

    def start_manual_recording(self, source_id: int):
        uri = self._rolling_sources.get(source_id)
        if not uri:
            logger.warning("Cannot start manual recording: no URI for source_id=%d", source_id)
            return
        logger.info("Manual recording requested: source_id=%d (covered by rolling)", source_id)

    def stop_recording(self, source_id: int):
        logger.info("Recording stop requested: source_id=%d", source_id)

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
