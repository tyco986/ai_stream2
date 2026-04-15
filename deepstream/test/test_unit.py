"""Unit tests for StorageManager, DiskGuard, recording archival, resolve helpers, and clip extraction.

These tests use ``tmp_path`` to simulate the file system and do NOT
require a running DeepStream container.
"""

import os
import shutil
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from unittest.mock import MagicMock

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from utils.storage import StorageManager
from daemons.disk_guard import DiskGuard
from recording.clip_extractor import ClipExtractionError, RollingClipExtractor, parse_utc_iso


# =====================================================================
# StorageManager
# =====================================================================

class TestStorageManager:

    def test_buffer_dir_created(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        assert sm.buffer_dir.exists()
        assert sm.buffer_dir == tmp_path / "storage" / "recordings"

    def test_ensure_dirs_creates_per_camera(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")
        assert sm.rolling_dir("cam_001").is_dir()
        assert sm.locked_dir("cam_001").is_dir()
        assert sm.screenshots_dir("cam_001").is_dir()

    def test_ensure_dirs_idempotent(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")
        sm.ensure_dirs("cam_001")
        assert sm.rolling_dir("cam_001").is_dir()

    def test_dirs_for_disk_guard_cleanup(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")
        sm.ensure_dirs("cam_002")
        dirs = sm.dirs_for_disk_guard_cleanup()
        names = sorted(d.parent.name for d in dirs)
        assert names == ["cam_001", "cam_002"]

    def test_dirs_for_disk_guard_cleanup_excludes_buffer(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")
        dirs = sm.dirs_for_disk_guard_cleanup()
        assert all(d.parent.name != "recordings" for d in dirs)

    def test_dirs_for_disk_guard_includes_legacy_only(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        leg = sm.legacy_recordings_dir("legacy_cam")
        leg.mkdir(parents=True)
        dirs = sm.dirs_for_disk_guard_cleanup()
        assert leg in dirs

    def test_paths(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "s"))
        assert sm.rolling_dir("x") == tmp_path / "s" / "x" / "rolling"
        assert sm.locked_dir("x") == tmp_path / "s" / "x" / "locked"
        assert sm.legacy_recordings_dir("x") == tmp_path / "s" / "x" / "recordings"
        assert sm.screenshots_dir("x") == tmp_path / "s" / "x" / "screenshots"


# =====================================================================
# DiskGuard — buffer cleanup
# =====================================================================

class TestDiskGuardBuffer:

    def _make_guard(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        return DiskGuard(storage=sm, max_usage_percent=95, check_interval=9999), sm

    def test_stale_buffer_files_deleted(self, tmp_path):
        guard, sm = self._make_guard(tmp_path)
        stale = sm.buffer_dir / "old_segment.mp4"
        stale.write_bytes(b"\x00" * 100)
        os.utime(stale, (time.time() - 120, time.time() - 120))

        guard._cleanup_buffer()
        assert not stale.exists()

    def test_recent_buffer_files_kept(self, tmp_path):
        guard, sm = self._make_guard(tmp_path)
        recent = sm.buffer_dir / "active.mp4"
        recent.write_bytes(b"\x00" * 100)

        guard._cleanup_buffer()
        assert recent.exists()

    def test_empty_buffer_files_deleted_after_grace(self, tmp_path):
        guard, sm = self._make_guard(tmp_path)
        empty = sm.buffer_dir / "dead_sr.mp4"
        empty.write_bytes(b"")
        os.utime(empty, (time.time() - 90, time.time() - 90))

        guard._cleanup_buffer()
        assert not empty.exists()


# =====================================================================
# DiskGuard — capacity-based cleanup
# =====================================================================

class TestDiskGuardCapacity:

    def test_cleanup_by_capacity(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")

        rec_dir = sm.rolling_dir("cam_001")
        for i in range(5):
            f = rec_dir / f"seg_{i:03d}.mp4"
            f.write_bytes(b"\x00" * 1000)
            os.utime(f, (time.time() - 300 + i, time.time() - 300 + i))

        guard = DiskGuard(
            storage=sm,
            max_usage_percent=99,
            max_storage_bytes=3000,
            check_interval=9999,
        )
        guard._cleanup_by_capacity()

        remaining = list(rec_dir.glob("*.mp4"))
        total = sum(f.stat().st_size for f in remaining)
        assert total <= 3000

    def test_no_cleanup_when_under_limit(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")

        rec_dir = sm.rolling_dir("cam_001")
        for i in range(3):
            f = rec_dir / f"seg_{i:03d}.mp4"
            f.write_bytes(b"\x00" * 100)
            os.utime(f, (time.time() - 120, time.time() - 120))

        guard = DiskGuard(
            storage=sm,
            max_usage_percent=99,
            max_storage_bytes=999999,
            check_interval=9999,
        )
        guard._cleanup_by_capacity()

        remaining = list(rec_dir.glob("*.mp4"))
        assert len(remaining) == 3

    def test_no_cleanup_when_disabled(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")
        (sm.rolling_dir("cam_001") / "seg.mp4").write_bytes(b"\x00" * 9999)
        os.utime(
            sm.rolling_dir("cam_001") / "seg.mp4",
            (time.time() - 120, time.time() - 120),
        )

        guard = DiskGuard(
            storage=sm,
            max_usage_percent=99,
            max_storage_bytes=0,
            check_interval=9999,
        )
        guard._cleanup_by_capacity()
        assert (sm.rolling_dir("cam_001") / "seg.mp4").exists()


# =====================================================================
# DiskGuard — multi-camera cleanup order
# =====================================================================

class TestDiskGuardMultiCamera:

    def test_oldest_across_cameras_deleted_first(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_A")
        sm.ensure_dirs("cam_B")

        old_file = sm.rolling_dir("cam_A") / "old.mp4"
        old_file.write_bytes(b"\x00" * 2000)
        os.utime(old_file, (time.time() - 600, time.time() - 600))

        new_file = sm.rolling_dir("cam_B") / "new.mp4"
        new_file.write_bytes(b"\x00" * 2000)
        os.utime(new_file, (time.time() - 120, time.time() - 120))

        guard = DiskGuard(
            storage=sm,
            max_usage_percent=99,
            max_storage_bytes=2500,
            check_interval=9999,
        )
        guard._cleanup_by_capacity()

        assert not old_file.exists(), "Oldest file should be deleted first"
        assert new_file.exists(), "Newer file should be kept"


# =====================================================================
# Recording archival (_on_sr_done move)
# =====================================================================

class TestRecordingArchival:

    def _make_manager(self, storage):
        """Build a RollingRecordManager with mocked SmartRecord dependency."""
        import types

        class FakeSmartRecordController:
            def __init__(self, *args, **kwargs):
                pass
            def register_source(self, *args, **kwargs):
                pass
            def unregister_source(self, *args, **kwargs):
                pass
            def start(self, *args, **kwargs):
                pass
            def stop(self, *args, **kwargs):
                pass
            def stop_all(self):
                pass

        mock_sr_module = types.ModuleType("recording.smartrecord")
        mock_sr_module.SmartRecordController = FakeSmartRecordController
        sys.modules["recording.smartrecord"] = mock_sr_module
        sys.modules.setdefault("nvdssr_ext", types.ModuleType("nvdssr_ext"))

        if "recording.manager" in sys.modules:
            del sys.modules["recording.manager"]

        from recording.manager import RollingRecordManager
        return RollingRecordManager(
            storage=storage,
            segment_duration=300,
            source_element=None,
        )

    def test_on_sr_done_moves_file(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("cam_001")
        mgr = self._make_manager(sm)
        mgr._camera_map[0] = "cam_001"

        src_file = sm.buffer_dir / "segment_0000.mp4"
        src_file.write_bytes(b"\x00" * 500)

        mgr._on_sr_done(0, {
            "filename": "segment_0000.mp4",
            "dirpath": str(sm.buffer_dir),
        })

        assert not src_file.exists(), "Source file should be moved"
        dest = sm.rolling_dir("cam_001") / "segment_0000.mp4"
        assert dest.exists(), "File should appear in per-camera dir"
        assert dest.stat().st_size == 500

    def test_on_sr_done_no_camera_mapping(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        mgr = self._make_manager(sm)

        src_file = sm.buffer_dir / "orphan.mp4"
        src_file.write_bytes(b"\x00" * 100)

        mgr._on_sr_done(99, {
            "filename": "orphan.mp4",
            "dirpath": str(sm.buffer_dir),
        })
        assert src_file.exists(), "File should remain when no camera mapping"


# =====================================================================
# _resolve_camera_id / _resolve_source_id
# =====================================================================

class TestResolveHelpers:
    """Test the resolve methods without importing CommandConsumer (needs Kafka)."""

    @staticmethod
    def _resolve_source_id(source_map, source_ref) -> int:
        if isinstance(source_ref, int):
            if source_ref in source_map.values():
                return source_ref
            raise ValueError(f"Unknown source_id: {source_ref}")
        if isinstance(source_ref, str) and source_ref.isdigit():
            source_id = int(source_ref)
            if source_id in source_map.values():
                return source_id
            raise ValueError(f"Unknown source_id: {source_ref}")
        source_id = source_map.get(source_ref)
        if source_id is None:
            raise ValueError(f"Unknown sensor_id: {source_ref}")
        return source_id

    @staticmethod
    def _resolve_camera_id(source_map, source_ref) -> str:
        if isinstance(source_ref, str) and not source_ref.isdigit():
            if source_ref in source_map:
                return source_ref
            raise ValueError(f"Unknown sensor_id: {source_ref}")
        int_id = int(source_ref)
        for sensor_id, src_id in source_map.items():
            if src_id == int_id:
                return sensor_id
        raise ValueError(f"Cannot resolve camera_id for source_id: {source_ref}")

    def test_resolve_source_id_by_sensor_id(self):
        m = {"cam_001": 0, "cam_002": 1}
        assert self._resolve_source_id(m, "cam_001") == 0
        assert self._resolve_source_id(m, "cam_002") == 1

    def test_resolve_source_id_by_int(self):
        assert self._resolve_source_id({"cam_001": 0}, 0) == 0

    def test_resolve_source_id_by_numeric_string(self):
        assert self._resolve_source_id({"cam_001": 3}, "3") == 3

    def test_resolve_source_id_unknown(self):
        with pytest.raises(ValueError):
            self._resolve_source_id({"cam_001": 0}, "cam_999")

    def test_resolve_camera_id_by_sensor_string(self):
        m = {"cam_001": 0, "cam_002": 1}
        assert self._resolve_camera_id(m, "cam_001") == "cam_001"

    def test_resolve_camera_id_by_int(self):
        m = {"cam_001": 0, "cam_002": 1}
        assert self._resolve_camera_id(m, 1) == "cam_002"

    def test_resolve_camera_id_by_numeric_string(self):
        assert self._resolve_camera_id({"cam_001": 5}, "5") == "cam_001"

    def test_resolve_camera_id_unknown(self):
        with pytest.raises(ValueError):
            self._resolve_camera_id({"cam_001": 0}, 99)

    def test_resolve_camera_id_unknown_sensor(self):
        with pytest.raises(ValueError):
            self._resolve_camera_id({"cam_001": 0}, "cam_999")


# =====================================================================
# Mode A: parse_utc_iso + RollingClipExtractor
# =====================================================================

class TestParseUtcIso:

    def test_z_suffix(self):
        t = parse_utc_iso("2026-06-15T12:30:00Z")
        assert t.tzinfo is not None
        assert t.hour == 12 and t.minute == 30


class TestRollingClipExtractorEmpty:

    def test_no_segments_raises(self, tmp_path):
        sm = StorageManager(base_dir=str(tmp_path / "storage"))
        sm.ensure_dirs("c1")
        ex = RollingClipExtractor(sm)
        ws = datetime(2026, 1, 1, tzinfo=timezone.utc)
        we = datetime(2026, 1, 1, 1, tzinfo=timezone.utc)
        with pytest.raises(ClipExtractionError):
            ex.extract("c1", ws, we, "rid-1")
