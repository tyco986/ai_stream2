"""Per-source video recording using GStreamer gi bindings.

Pipeline.start_recording cannot find child nvurisrcbin elements inside
nvmultiurisrcbin, so we use standalone gi pipelines that read from the
camera's RTSP URL and write MP4 segments via splitmuxsink.
"""

import logging
import threading
from pathlib import Path

import gi

gi.require_version("Gst", "1.0")
from gi.repository import GLib, Gst  # noqa: E402

Gst.init(None)

logger = logging.getLogger(__name__)

_SEGMENT_NS = 300 * Gst.SECOND  # 5-minute segments


class _RecordingSession:
    """One GStreamer pipeline that records a single RTSP source to disk."""

    def __init__(self, source_id: int, uri: str, output_dir: Path,
                 segment_duration_ns: int, on_file_done=None):
        self._source_id = source_id
        self._uri = uri
        self._output_dir = output_dir
        self._on_file_done = on_file_done
        self._pipeline = None
        self._loop = None
        self._thread = None

        self._output_dir.mkdir(parents=True, exist_ok=True)
        self._build_pipeline(segment_duration_ns)

    def _build_pipeline(self, segment_ns: int):
        location = str(self._output_dir / f"src{self._source_id}_%05d.mp4")

        pipeline_str = (
            f'rtspsrc location="{self._uri}" latency=200 protocols=tcp '
            f"! rtph264depay ! h264parse "
            f'! splitmuxsink name=splitmux location="{location}" '
            f"max-size-time={segment_ns} muxer-factory=mp4mux "
            f"async-finalize=true"
        )
        self._pipeline = Gst.parse_launch(pipeline_str)

        bus = self._pipeline.get_bus()
        bus.add_signal_watch()
        bus.connect("message::error", self._on_error)
        bus.connect("message::eos", self._on_eos)

        splitmux = self._pipeline.get_by_name("splitmux")
        if splitmux and self._on_file_done:
            splitmux.connect("format-location-full", self._on_format_location)

    def _on_format_location(self, splitmux, fragment_id, first_sample):
        location = str(self._output_dir / f"src{self._source_id}_{fragment_id:05d}.mp4")
        logger.info("Recording new segment: %s (source_id=%d)", location, self._source_id)
        return location

    def _on_error(self, bus, msg):
        err, debug = msg.parse_error()
        logger.error(
            "Recording pipeline error (source_id=%d): %s [%s]",
            self._source_id, err.message, debug,
        )
        self.stop()

    def _on_eos(self, bus, msg):
        logger.info("Recording EOS (source_id=%d)", self._source_id)
        self.stop()

    def start(self):
        self._pipeline.set_state(Gst.State.PLAYING)
        self._loop = GLib.MainLoop()
        self._thread = threading.Thread(
            target=self._loop.run, daemon=True,
            name=f"rec-src{self._source_id}",
        )
        self._thread.start()
        logger.info(
            "Recording started: source_id=%d uri=%s dir=%s",
            self._source_id, self._uri, self._output_dir,
        )

    def stop(self):
        if self._pipeline:
            self._pipeline.send_event(Gst.Event.new_eos())
            self._pipeline.set_state(Gst.State.NULL)
            self._pipeline = None
        if self._loop and self._loop.is_running():
            self._loop.quit()
        logger.info("Recording stopped: source_id=%d", self._source_id)


class GiRecorder:
    """Manage per-source recording sessions using gi GStreamer pipelines."""

    def __init__(self, output_dir: str, segment_duration: int = 300):
        self._output_dir = Path(output_dir)
        self._segment_ns = segment_duration * Gst.SECOND
        self._sessions = {}  # source_id → _RecordingSession
        self._lock = threading.Lock()

    def start(self, source_id: int, uri: str):
        with self._lock:
            if source_id in self._sessions:
                logger.warning("Recording already active for source_id=%d", source_id)
                return
            session = _RecordingSession(
                source_id=source_id,
                uri=uri,
                output_dir=self._output_dir,
                segment_duration_ns=self._segment_ns,
            )
            self._sessions[source_id] = session
            session.start()

    def stop(self, source_id: int):
        with self._lock:
            session = self._sessions.pop(source_id, None)
        if session:
            session.stop()

    def stop_all(self):
        with self._lock:
            sessions = list(self._sessions.values())
            self._sessions.clear()
        for session in sessions:
            session.stop()
