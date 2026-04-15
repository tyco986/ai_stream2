import logging
from pathlib import Path

logger = logging.getLogger(__name__)


class StorageManager:
    """Centralized storage path management for recordings and screenshots.

    Directory layout::

        {base_dir}/
        ├── recordings/              ← SmartRecord global buffer
        ├── {camera_id}/
        │   ├── rolling/             ← Rolling (7x24) segments after archive
        │   ├── locked/              ← Event / manual clips (protected from rolling cleanup)
        │   └── screenshots/
        └── ...

    Legacy layout (still discovered for DiskGuard until migrated)::

        {camera_id}/recordings/      ← older deployments; cleaned same as rolling
    """

    def __init__(self, base_dir: str = "/app/storage"):
        self._base = Path(base_dir)
        self._buffer_dir = self._base / "recordings"
        self._buffer_dir.mkdir(parents=True, exist_ok=True)

    @property
    def base_dir(self) -> Path:
        return self._base

    @property
    def buffer_dir(self) -> Path:
        """SmartRecord global write directory (temporary buffer)."""
        return self._buffer_dir

    def rolling_dir(self, camera_id: str) -> Path:
        """Archived rolling-record segments (subject to DiskGuard recycling)."""
        return self._base / camera_id / "rolling"

    def locked_dir(self, camera_id: str) -> Path:
        """Event / manual recordings (not subject to rolling DiskGuard deletion)."""
        return self._base / camera_id / "locked"

    def legacy_recordings_dir(self, camera_id: str) -> Path:
        """Pre-split per-camera flat archive (discovery only, not created by ensure_dirs)."""
        return self._base / camera_id / "recordings"

    def screenshots_dir(self, camera_id: str) -> Path:
        return self._base / camera_id / "screenshots"

    def ensure_dirs(self, camera_id: str):
        self.rolling_dir(camera_id).mkdir(parents=True, exist_ok=True)
        self.locked_dir(camera_id).mkdir(parents=True, exist_ok=True)
        self.screenshots_dir(camera_id).mkdir(parents=True, exist_ok=True)
        logger.info("Storage dirs ensured for camera_id=%s", camera_id)

    def dirs_for_disk_guard_cleanup(self) -> list[Path]:
        """Return per-camera dirs whose .mp4 files may be deleted by DiskGuard.

        Includes ``{camera}/rolling`` and legacy ``{camera}/recordings`` (not ``locked``).
        """
        dirs = []
        if not self._base.exists():
            return dirs
        for child in self._base.iterdir():
            if not child.is_dir() or child.name == "recordings":
                continue
            rolling = child / "rolling"
            legacy = child / "recordings"
            if rolling.is_dir():
                dirs.append(rolling)
            elif legacy.is_dir():
                dirs.append(legacy)
        return dirs
