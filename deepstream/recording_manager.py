import logging
import shutil
from pathlib import Path

logger = logging.getLogger(__name__)


class RollingRecordManager:
    """Manage rolling (7x24), event, and manual recordings via SmartRecord.

    Design (dashcam model):
        - Rolling segments stay in ``rolling/`` and are cleaned by DiskGuard.
        - Event / manual recordings are moved to ``locked/`` on sr-done so
          DiskGuard's rolling cleanup never touches them.

    Thread safety:
        ``start_rolling`` / ``stop_rolling`` are called from the on_message
        callback (GStreamer main loop).  ``start_event_recording``,
        ``start_manual_recording``, ``stop_recording`` are called from
        CommandConsumer (daemon thread).  ``on_sr_done`` is called from the
        GStreamer signal handler.
        GObject signal ``emit()`` and ``set_property()`` are thread-safe.
    """

    SEGMENT_DURATION = 300  # seconds (5 min)

    def __init__(self, sr_controller, rolling_dir, locked_dir,
                 segment_duration=None):
        self._sr = sr_controller
        self._rolling_dir = Path(rolling_dir)
        self._locked_dir = Path(locked_dir)
        self._rolling_sources = set()
        self._recording_type = {}  # source_id → "rolling" | "event" | "manual"

        if segment_duration is not None:
            self.SEGMENT_DURATION = segment_duration

        self._rolling_dir.mkdir(parents=True, exist_ok=True)
        self._locked_dir.mkdir(parents=True, exist_ok=True)

    # ------------------------------------------------------------------
    # rolling (7x24)
    # ------------------------------------------------------------------

    def start_rolling(self, source_id: int):
        self._rolling_sources.add(source_id)
        self._recording_type[source_id] = "rolling"
        self._start_segment(source_id)
        logger.info("Rolling recording started for source_id=%d", source_id)

    def stop_rolling(self, source_id: int):
        self._rolling_sources.discard(source_id)
        self._recording_type.pop(source_id, None)
        self._sr.emit("stop-sr", source_id)
        logger.info("Rolling recording stopped for source_id=%d", source_id)

    # ------------------------------------------------------------------
    # event recording (alert-triggered)
    # ------------------------------------------------------------------

    def start_event_recording(self, source_id: int, duration: int = 20):
        self._recording_type[source_id] = "event"
        self._sr.emit("start-sr", source_id, duration)
        logger.info("Event recording started: source_id=%d duration=%ds", source_id, duration)

    # ------------------------------------------------------------------
    # manual recording (user-controlled)
    # ------------------------------------------------------------------

    def start_manual_recording(self, source_id: int):
        self._recording_type[source_id] = "manual"
        self._sr.emit("start-sr", source_id, 0)
        logger.info("Manual recording started: source_id=%d", source_id)

    def stop_recording(self, source_id: int):
        self._sr.emit("stop-sr", source_id)
        logger.info("Recording stopped: source_id=%d", source_id)

    # ------------------------------------------------------------------
    # sr-done callback
    # ------------------------------------------------------------------

    def on_sr_done(self, source_id: int, filepath: str):
        """Called when SmartRecord finishes writing a segment.

        Event / manual recordings are moved to ``locked/`` to protect them
        from DiskGuard's rolling cleanup.  Rolling segments stay in place
        and the next segment is started immediately (chain recording).
        """
        filepath = Path(filepath)
        rec_type = self._recording_type.get(source_id, "rolling")

        if rec_type in ("event", "manual"):
            dest = self._locked_dir / filepath.name
            shutil.move(str(filepath), str(dest))
            logger.info("Locked recording moved: %s → %s", filepath, dest)
            self._recording_type[source_id] = "rolling"

        if source_id in self._rolling_sources:
            self._recording_type[source_id] = "rolling"
            self._start_segment(source_id)

    # ------------------------------------------------------------------
    # internal
    # ------------------------------------------------------------------

    def _start_segment(self, source_id: int):
        self._sr.emit("start-sr", source_id, self.SEGMENT_DURATION)
