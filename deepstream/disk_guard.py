import logging
import shutil
import time
from pathlib import Path

logger = logging.getLogger(__name__)


class DiskGuard:
    """Daemon thread that keeps disk usage under control.

    Two independent strategies run on each tick:

    1. **Rolling cleanup** -- when the partition holding ``rolling_dir``
       exceeds ``max_usage_percent`` usage, delete the oldest ``.mp4``
       files in ``rolling_dir`` until usage drops below the threshold.
       Files modified in the last 60 s are skipped (active writes).

    2. **Locked cleanup** -- delete files in ``locked_dir`` older than
       ``locked_max_age_days`` regardless of disk pressure.
    """

    ACTIVE_WRITE_GRACE_SEC = 60

    def __init__(self, rolling_dir, locked_dir,
                 max_usage_percent=85,
                 locked_max_age_days=30,
                 check_interval=60):
        self._rolling_dir = Path(rolling_dir)
        self._locked_dir = Path(locked_dir)
        self._max_pct = max_usage_percent
        self._max_age_sec = locked_max_age_days * 86400
        self._interval = check_interval

    # ------------------------------------------------------------------
    # main loop (called via daemon thread)
    # ------------------------------------------------------------------

    def run(self):
        logger.info(
            "DiskGuard started: rolling=%s locked=%s max_usage=%d%%",
            self._rolling_dir, self._locked_dir, self._max_pct,
        )
        while True:
            try:
                self._cleanup_rolling_by_usage()
                self._cleanup_locked_by_age()
            except Exception:
                logger.exception("DiskGuard tick failed")
            time.sleep(self._interval)

    # ------------------------------------------------------------------
    # rolling cleanup
    # ------------------------------------------------------------------

    def _cleanup_rolling_by_usage(self):
        usage = shutil.disk_usage(self._rolling_dir)
        used_pct = (usage.used / usage.total) * 100
        if used_pct <= self._max_pct:
            return

        logger.warning("Disk usage %.1f%% > %d%%, cleaning rolling recordings", used_pct, self._max_pct)
        now = time.time()
        files = sorted(
            self._rolling_dir.glob("*.mp4"),
            key=lambda p: p.stat().st_mtime,
        )

        for path in files:
            if now - path.stat().st_mtime < self.ACTIVE_WRITE_GRACE_SEC:
                continue
            path.unlink()
            logger.info("Deleted rolling recording: %s", path)
            usage = shutil.disk_usage(self._rolling_dir)
            if (usage.used / usage.total) * 100 <= self._max_pct:
                break

    # ------------------------------------------------------------------
    # locked cleanup
    # ------------------------------------------------------------------

    def _cleanup_locked_by_age(self):
        now = time.time()
        for path in self._locked_dir.glob("*.mp4"):
            age = now - path.stat().st_mtime
            if age > self._max_age_sec:
                path.unlink()
                logger.info("Deleted aged locked recording: %s (%.1f days)", path, age / 86400)
