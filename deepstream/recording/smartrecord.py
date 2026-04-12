"""SmartRecord controller using the nvdssr_ext C extension.

Extracts GstElement pointers from pyservicemaker Nodes, finds child
nvurisrcbin elements inside nvmultiurisrcbin, and emits start-sr /
stop-sr via the C bridge.
"""

import ctypes
import logging
import threading
import time

import nvdssr_ext

logger = logging.getLogger(__name__)

_REGISTER_RETRIES = 10
_REGISTER_INTERVAL = 0.5

_gobj_lib = ctypes.CDLL("libgobject-2.0.so.0")
_gst_lib = ctypes.CDLL("libgstreamer-1.0.so.0")
_gst_lib.gst_element_get_state.argtypes = [
    ctypes.c_void_p, ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int), ctypes.c_uint64,
]
_gst_lib.gst_element_get_state.restype = ctypes.c_int

_GST_STATE_PLAYING = 4
_ONE_SEC_NS = 1_000_000_000


def _gobject_get_uint(ptr: int, prop: str) -> int:
    val = ctypes.c_uint(0)
    _gobj_lib.g_object_get(
        ctypes.c_void_p(ptr), prop.encode(),
        ctypes.byref(val), ctypes.c_void_p(0),
    )
    return val.value


def _gobject_get_str(ptr: int, prop: str) -> str:
    val = ctypes.c_char_p()
    _gobj_lib.g_object_get(
        ctypes.c_void_p(ptr), prop.encode(),
        ctypes.byref(val), ctypes.c_void_p(0),
    )
    return val.value.decode() if val.value else "(null)"


class SmartRecordController:
    """Control SmartRecord on nvurisrcbin children of nvmultiurisrcbin.

    Works entirely with raw GstElement pointers (as integers) obtained
    from the nvdssr_ext C extension, bypassing PyGObject limitations.
    """

    def __init__(self, source_node, on_recording_done=None):
        self._on_recording_done = on_recording_done
        self._lock = threading.Lock()
        self._sources = {}       # source_id -> nvurisrcbin ptr (int)
        self._sessions = {}      # source_id -> session_id

        self._parent_ptr = nvdssr_ext.extract_gst_element(source_node)
        parent_name = nvdssr_ext.get_element_name(self._parent_ptr)
        logger.info(
            "SmartRecordController: parent element '%s' at %#x",
            parent_name, self._parent_ptr,
        )

    def __del__(self):
        for ptr in self._sources.values():
            nvdssr_ext.release_gst_element(ptr)
        if hasattr(self, "_parent_ptr") and self._parent_ptr:
            nvdssr_ext.release_gst_element(self._parent_ptr)

    # ------------------------------------------------------------------
    # source registration
    # ------------------------------------------------------------------

    def register_source(self, source_id: int):
        with self._lock:
            if source_id in self._sources:
                return True

        child_ptr = None
        for attempt in range(_REGISTER_RETRIES):
            child_ptr = nvdssr_ext.find_child_nvurisrcbin(
                self._parent_ptr, source_id,
            )
            if child_ptr is not None:
                break
            logger.debug(
                "register_source: attempt %d/%d — nvurisrcbin not found for source_id=%d",
                attempt + 1, _REGISTER_RETRIES, source_id,
            )
            time.sleep(_REGISTER_INTERVAL)

        if child_ptr is None:
            logger.warning(
                "register_source: no nvurisrcbin found for source_id=%d after %d retries",
                source_id, _REGISTER_RETRIES,
            )
            return False

        name = nvdssr_ext.get_element_name(child_ptr)
        with self._lock:
            self._sources[source_id] = child_ptr

        self._log_sr_properties(child_ptr, name, source_id)
        return True

    def _log_sr_properties(self, elem_ptr, name, source_id):
        """Log SmartRecord properties on the child nvurisrcbin for debugging."""
        logger.info(
            "Registered nvurisrcbin '%s' at %#x for source_id=%d  "
            "smart-record=%d  dir=%s  prefix=%s  cache=%d  default-dur=%d",
            name, elem_ptr, source_id,
            _gobject_get_uint(elem_ptr, "smart-record"),
            _gobject_get_str(elem_ptr, "smart-rec-dir-path"),
            _gobject_get_str(elem_ptr, "smart-rec-file-prefix"),
            _gobject_get_uint(elem_ptr, "smart-rec-cache"),
            _gobject_get_uint(elem_ptr, "smart-rec-default-duration"),
        )

    def unregister_source(self, source_id: int):
        with self._lock:
            ptr = self._sources.pop(source_id, None)
            self._sessions.pop(source_id, None)
        if ptr:
            nvdssr_ext.release_gst_element(ptr)

    # ------------------------------------------------------------------
    # recording control
    # ------------------------------------------------------------------

    def start(self, source_id: int, start_time: int = 0,
              duration: int = 300) -> int:
        with self._lock:
            elem_ptr = self._sources.get(source_id)
            if not elem_ptr:
                logger.warning("start: source_id=%d not registered", source_id)
                return -1
            if source_id in self._sessions:
                logger.warning(
                    "start: recording already active for source_id=%d (session=%d)",
                    source_id, self._sessions[source_id],
                )
                return self._sessions[source_id]

        if not self._wait_for_playing(elem_ptr, source_id):
            return -1

        session_id = nvdssr_ext.start_recording(elem_ptr, start_time, duration)
        with self._lock:
            self._sessions[source_id] = session_id

        logger.info(
            "SmartRecord started: source_id=%d session_id=%d start_time=%d duration=%d",
            source_id, session_id, start_time, duration,
        )
        return session_id

    def _wait_for_playing(self, elem_ptr, source_id, timeout=15):
        """Wait for the element to reach PLAYING state before recording."""
        state = ctypes.c_int(0)
        for _ in range(timeout * 2):
            _gst_lib.gst_element_get_state(
                ctypes.c_void_p(elem_ptr),
                ctypes.byref(state), None,
                ctypes.c_uint64(_ONE_SEC_NS // 2),
            )
            if state.value == _GST_STATE_PLAYING:
                return True
            time.sleep(0.5)

        logger.warning(
            "start: source_id=%d element not in PLAYING state (state=%d) after %ds",
            source_id, state.value, timeout,
        )
        return False

    def stop(self, source_id: int):
        with self._lock:
            elem_ptr = self._sources.get(source_id)
            session_id = self._sessions.pop(source_id, None)

        if elem_ptr is None or session_id is None:
            logger.warning("stop: no active session for source_id=%d", source_id)
            return

        nvdssr_ext.stop_recording(elem_ptr, session_id)
        logger.info(
            "SmartRecord stopped: source_id=%d session_id=%d",
            source_id, session_id,
        )

    def stop_all(self):
        with self._lock:
            pairs = list(self._sessions.items())
        for source_id, _ in pairs:
            self.stop(source_id)
