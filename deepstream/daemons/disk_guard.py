import logging
import shutil
import threading
import time
from pathlib import Path

from utils.storage import StorageManager

logger = logging.getLogger(__name__)


class DiskGuard:
    """Daemon thread that keeps disk usage under control.

    Two independent thresholds (whichever triggers first):

    1. **Percentage** -- when the partition holding the storage directory
       exceeds ``max_usage_percent``, delete the oldest ``.mp4`` files
       across all per-camera recording directories until usage drops.

    2. **Absolute capacity** -- when the total size of all recording
       files exceeds ``max_storage_bytes``, delete the oldest files
       until total size drops below the limit.

    Files modified in the last 60 s are skipped (active writes).
    The SmartRecord buffer directory is also cleaned of stale files.
    """

    ACTIVE_WRITE_GRACE_SEC = 60

    def __init__(self, storage: StorageManager,
                 max_usage_percent: int = 85,
                 max_storage_bytes: int = 0,
                 check_interval: int = 60):
        self._storage = storage
        self._max_pct = max_usage_percent
        self._max_bytes = max_storage_bytes
        self._interval = check_interval
        self._shutdown = threading.Event()

    # ------------------------------------------------------------------
    # main loop (called via daemon thread)
    # ------------------------------------------------------------------

    def run(self):
        logger.info(
            "DiskGuard started: storage=%s max_usage=%d%% max_bytes=%s",
            self._storage.base_dir, self._max_pct,
            f"{self._max_bytes / (1024**3):.1f}GB" if self._max_bytes else "unlimited",
        )
        while not self._shutdown.wait(timeout=self._interval):
            try:
                self._cleanup_buffer()
                self._cleanup_by_usage()
                self._cleanup_by_capacity()
            except Exception:
                logger.exception("DiskGuard tick failed")

    def stop(self):
        self._shutdown.set()

    # ------------------------------------------------------------------
    # buffer cleanup — stale files in SmartRecord global dir
    # ------------------------------------------------------------------

    def _cleanup_buffer(self):
        now = time.time()
        for path in self._storage.buffer_dir.glob("*.mp4"):
            if now - path.stat().st_mtime > self.ACTIVE_WRITE_GRACE_SEC:
                path.unlink()
                logger.info("Deleted stale buffer file: %s", path)

    # ------------------------------------------------------------------
    # percentage-based cleanup
    # ------------------------------------------------------------------

    def _cleanup_by_usage(self):
        usage = shutil.disk_usage(self._storage.base_dir)
        used_pct = (usage.used / usage.total) * 100
        if used_pct <= self._max_pct:
            return

        logger.warning("Disk usage %.1f%% > %d%%, cleaning recordings", used_pct, self._max_pct)
        for path in self._oldest_recordings():
            path.unlink()
            logger.info("Deleted recording (usage): %s", path)
            usage = shutil.disk_usage(self._storage.base_dir)
            if (usage.used / usage.total) * 100 <= self._max_pct:
                break

    # ------------------------------------------------------------------
    # absolute capacity cleanup
    # ------------------------------------------------------------------

    def _cleanup_by_capacity(self):
        if self._max_bytes <= 0:
            return

        total_size = self._total_recording_size()
        if total_size <= self._max_bytes:
            return

        logger.warning(
            "Recording size %.1fGB > %.1fGB limit, cleaning",
            total_size / (1024**3), self._max_bytes / (1024**3),
        )
        for path in self._oldest_recordings():
            freed = path.stat().st_size
            path.unlink()
            total_size -= freed
            logger.info("Deleted recording (capacity): %s", path)
            if total_size <= self._max_bytes:
                break

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def _oldest_recordings(self) -> list[Path]:
        """Collect all .mp4 files across per-camera dirs, sorted oldest first."""
        now = time.time()
        files = []
        for rec_dir in self._storage.all_recording_dirs():
            for path in rec_dir.glob("*.mp4"):
                if now - path.stat().st_mtime < self.ACTIVE_WRITE_GRACE_SEC:
                    continue
                files.append(path)
        files.sort(key=lambda p: p.stat().st_mtime)
        return files

    def _total_recording_size(self) -> int:
        total = 0
        for rec_dir in self._storage.all_recording_dirs():
            for path in rec_dir.glob("*.mp4"):
                total += path.stat().st_size
        return total
