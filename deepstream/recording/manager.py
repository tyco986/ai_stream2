import logging
import shutil
from pathlib import Path

from recording.smartrecord import SmartRecordController
from utils.storage import StorageManager

logger = logging.getLogger(__name__)


class RollingRecordManager:
    """Manage rolling (7x24) recordings via SmartRecord.

    SmartRecord writes segments to a global buffer directory.  When a
    segment finishes (``_on_sr_done``), the file is moved to the
    per-camera archive: ``storage/{camera_id}/recordings/``.
    """

    SEGMENT_DURATION = 300  # seconds (5 min)

    def __init__(self, storage: StorageManager, segment_duration=None,
                 source_element=None):
        self._storage = storage
        self._rolling_sources = {}   # source_id → uri
        self._uri_map = {}           # source_id → uri (persists after stop_rolling)
        self._camera_map = {}        # source_id → camera_id
        self._segment_duration = segment_duration or self.SEGMENT_DURATION

        self._sr_controller = SmartRecordController(
            source_element,
            on_recording_done=self._on_sr_done,
        )
        logger.info("Recording backend: smartrecord (C extension)")

    # ------------------------------------------------------------------
    # source lifecycle
    # ------------------------------------------------------------------

    def register_source(self, source_id: int, camera_id: str, uri: str):
        self._uri_map[source_id] = uri
        self._camera_map[source_id] = camera_id
        self._storage.ensure_dirs(camera_id)
        self._sr_controller.register_source(source_id)

    def unregister_source(self, source_id: int):
        self._uri_map.pop(source_id, None)
        self._camera_map.pop(source_id, None)
        self._sr_controller.unregister_source(source_id)

    # ------------------------------------------------------------------
    # rolling (7x24)
    # ------------------------------------------------------------------

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
        filename = info.get("filename", "")
        dirpath = info.get("dirpath", "")
        src_path = Path(dirpath) / filename if dirpath and filename else None

        camera_id = self._camera_map.get(source_id)
        if src_path and src_path.exists() and camera_id:
            dest_dir = self._storage.recordings_dir(camera_id)
            dest_dir.mkdir(parents=True, exist_ok=True)
            dest_path = dest_dir / filename
            shutil.move(str(src_path), str(dest_path))
            logger.info(
                "Recording archived: source_id=%d camera=%s file=%s",
                source_id, camera_id, dest_path,
            )
        else:
            logger.info(
                "SmartRecord segment done: source_id=%d file=%s/%s (no archive)",
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
