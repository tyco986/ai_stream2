import logging
from pathlib import Path

logger = logging.getLogger(__name__)


class StorageManager:
    """Centralized storage path management for recordings and screenshots.

    Directory layout::

        {base_dir}/
        ├── recordings/              ← SmartRecord 全局缓冲区
        ├── {camera_id}/
        │   ├── recordings/          ← 归档后的录像段
        │   └── screenshots/         ← 截图
        └── ...
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

    def recordings_dir(self, camera_id: str) -> Path:
        return self._base / camera_id / "recordings"

    def screenshots_dir(self, camera_id: str) -> Path:
        return self._base / camera_id / "screenshots"

    def ensure_dirs(self, camera_id: str):
        self.recordings_dir(camera_id).mkdir(parents=True, exist_ok=True)
        self.screenshots_dir(camera_id).mkdir(parents=True, exist_ok=True)
        logger.info("Storage dirs ensured for camera_id=%s", camera_id)

    def all_recording_dirs(self) -> list[Path]:
        """Return all per-camera recordings directories that exist."""
        dirs = []
        if not self._base.exists():
            return dirs
        for child in self._base.iterdir():
            if child.is_dir() and child.name != "recordings":
                rec_dir = child / "recordings"
                if rec_dir.is_dir():
                    dirs.append(rec_dir)
        return dirs
