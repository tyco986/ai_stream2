import logging
import os
import re
import shutil
import threading
import time
from pathlib import Path

from recording.smartrecord import SmartRecordController
from utils.storage import StorageManager

logger = logging.getLogger(__name__)

_SR_NAME = re.compile(r"^sr_(\d+)_")


class RollingRecordManager:
    """Manage rolling (7x24) recordings via SmartRecord.

    SmartRecord writes segments to a global buffer directory.  When a
    segment finishes (``_on_sr_done``), the file is moved to the
    per-camera rolling archive: ``storage/{camera_id}/rolling/``.
    Event/manual clips (future) go to ``storage/{camera_id}/locked/``.
    """

    SEGMENT_DURATION = 300  # seconds (5 min)

    def __init__(self, storage: StorageManager, segment_duration=None,
                 source_element=None):
        self._storage = storage
        self._rolling_sources = {}   # source_id → uri
        self._uri_map = {}           # source_id → uri (persists after stop_rolling)
        self._camera_map = {}        # source_id → camera_id
        self._segment_duration = segment_duration or self.SEGMENT_DURATION
        self._buffer_poll_interval = float(os.environ.get("DS_BUFFER_ARCHIVE_POLL_SEC", "10"))
        self._buffer_min_age = float(os.environ.get("DS_BUFFER_ARCHIVE_MIN_AGE_SEC", "45"))
        self._shutdown = threading.Event()
        self._archive_thread = threading.Thread(
            target=self._buffer_archive_loop,
            daemon=True,
            name="rolling-buffer-archive",
        )

        self._sr_controller = SmartRecordController(
            source_element,
            on_recording_done=self._on_sr_done,
        )
        logger.info("Recording backend: smartrecord (C extension)")
        self._archive_thread.start()

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
            try:
                sz = src_path.stat().st_size
            except OSError:
                sz = -1
            if sz == 0:
                logger.warning(
                    "Dropping empty SmartRecord segment (no media bytes): source_id=%s path=%s",
                    source_id,
                    src_path,
                )
                try:
                    src_path.unlink()
                except OSError:
                    pass
            elif sz > 0:
                dest_dir = self._storage.rolling_dir(camera_id)
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
    # buffer poll (sr-done is not wired to Python yet; archive completed files)
    # ------------------------------------------------------------------

    def _buffer_archive_loop(self):
        while not self._shutdown.wait(timeout=self._buffer_poll_interval):
            self._poll_buffer_archives()

    def _poll_buffer_archives(self):
        buf = self._storage.buffer_dir
        if not buf.is_dir():
            return
        now = time.time()
        for path in buf.glob("sr_*.mp4"):
            m = _SR_NAME.match(path.name)
            if not m:
                continue
            try:
                source_id = int(m.group(1))
            except ValueError:
                continue
            try:
                st = path.stat()
            except OSError:
                continue
            if st.st_size <= 0:
                continue
            if now - st.st_mtime < self._buffer_min_age:
                continue
            if source_id not in self._camera_map:
                continue
            self._on_sr_done(
                source_id,
                {"filename": path.name, "dirpath": str(buf)},
            )

    # ------------------------------------------------------------------
    # shutdown
    # ------------------------------------------------------------------

    def shutdown(self):
        self._shutdown.set()
        self._archive_thread.join(timeout=5)
        self._sr_controller.stop_all()
