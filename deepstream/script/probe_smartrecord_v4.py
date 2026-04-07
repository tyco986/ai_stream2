#!/usr/bin/env python3
"""Probe v4: Emit start-sr with correct gpointer parameters.

start-sr signature: (gpointer session_id_out, guint start_time, guint duration, gpointer user_data)
stop-sr signature: (guint session_id)
sr-done signature: (gpointer session_info, gpointer user_data)
"""

import ctypes
import logging
import os
import time

import requests

import gi
gi.require_version("Gst", "1.0")
from gi.repository import Gst, GLib, GObject

Gst.init(None)

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger("probe-sr-v4")

REST_PORT = 9001
REST_BASE = f"http://127.0.0.1:{REST_PORT}"
CAMERA_URL = "rtsp://127.0.0.1:8555/video1"
CAMERA_ID = "probe_cam_v4"
RECORDING_DIR = "/tmp/sr_probe_recordings_v4"


def _find_nvurisrcbin(src_bin):
    it = src_bin.iterate_recurse()
    while True:
        ret, child = it.next()
        if ret == Gst.IteratorResult.OK:
            factory = child.get_factory()
            if factory and factory.get_name() == "nvurisrcbin":
                return child
        elif ret in (Gst.IteratorResult.DONE, Gst.IteratorResult.ERROR):
            break
        elif ret == Gst.IteratorResult.RESYNC:
            it.resync()
    return None


def _sr_done_callback(element, session_info, user_data):
    logger.info("sr-done callback! element=%s", element.get_name())


def run_test():
    os.makedirs(RECORDING_DIR, exist_ok=True)

    # Clean up old files
    for f in os.listdir(RECORDING_DIR):
        os.remove(os.path.join(RECORDING_DIR, f))

    pipeline = Gst.Pipeline.new("sr-v4")

    src = Gst.ElementFactory.make("nvmultiurisrcbin", "src")
    src.set_property("ip-address", "0.0.0.0")
    src.set_property("port", REST_PORT)
    src.set_property("max-batch-size", 4)
    src.set_property("batched-push-timeout", 33333)
    src.set_property("width", 1920)
    src.set_property("height", 1080)
    src.set_property("live-source", 1)
    src.set_property("drop-pipeline-eos", 1)
    src.set_property("async-handling", 1)
    src.set_property("select-rtp-protocol", 0)
    src.set_property("latency", 100)
    src.set_property("file-loop", 1)
    src.set_property("smart-record", 2)
    src.set_property("smart-rec-dir-path", RECORDING_DIR)
    src.set_property("smart-rec-file-prefix", "probe_rec")
    src.set_property("smart-rec-cache", 10)
    src.set_property("smart-rec-container", 0)
    src.set_property("smart-rec-mode", 1)
    src.set_property("smart-rec-default-duration", 15)

    sink = Gst.ElementFactory.make("fakesink", "sink")
    sink.set_property("sync", 0)
    sink.set_property("async", 0)

    pipeline.add(src)
    pipeline.add(sink)
    src.link(sink)

    ret = pipeline.set_state(Gst.State.PLAYING)
    logger.info("Pipeline -> PLAYING: %s", ret)
    time.sleep(3)

    # Add stream
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_name": "Probe Camera V4",
            "camera_url": CAMERA_URL,
            "change": "camera_add",
        },
    }
    r = requests.post(f"{REST_BASE}/api/v1/stream/add", json=payload, timeout=10)
    logger.info("Add stream: %d", r.status_code)
    time.sleep(8)

    uribin = _find_nvurisrcbin(src)
    if not uribin:
        logger.error("No nvurisrcbin found!")
        pipeline.set_state(Gst.State.NULL)
        return

    logger.info("Found: %s", uribin.get_name())

    # Connect to sr-done callback
    uribin.connect("sr-done", _sr_done_callback)

    # ========================================
    # Approach 1: Use None for gpointer (NULL pointer)
    # ========================================
    logger.info("--- Approach 1: None for gpointer ---")
    try:
        uribin.emit("start-sr", None, 0, 15, None)
        sr = uribin.get_property("smart-rec-status")
        logger.info("Approach 1 result: smart-rec-status=%s", sr)
    except Exception as e:
        logger.warning("Approach 1 failed: %s", e)

    time.sleep(2)

    # ========================================
    # Approach 2: Use ctypes pointer for session ID
    # ========================================
    logger.info("--- Approach 2: ctypes c_uint pointer ---")
    try:
        session_id = ctypes.c_uint(0)
        session_ptr = ctypes.pointer(session_id)
        # Convert to int (memory address) - this is what gpointer expects
        ptr_val = ctypes.addressof(session_id)
        uribin.emit("start-sr", ptr_val, 0, 15, 0)
        sr = uribin.get_property("smart-rec-status")
        logger.info("Approach 2 result: smart-rec-status=%s, session_id=%d",
                     sr, session_id.value)
    except Exception as e:
        logger.warning("Approach 2 failed: %s", e)

    time.sleep(2)

    # ========================================
    # Approach 3: Use GLib.Variant or bytes
    # ========================================
    logger.info("--- Approach 3: various gpointer representations ---")
    for p0 in [b'\x00\x00\x00\x00', bytearray(4), GLib.Bytes.new(b'\x00' * 4)]:
        try:
            uribin.emit("start-sr", p0, 0, 15, None)
            sr = uribin.get_property("smart-rec-status")
            logger.info("Approach 3 (%s): smart-rec-status=%s", type(p0).__name__, sr)
            if sr:
                break
        except Exception as e:
            logger.warning("Approach 3 (%s) failed: %s", type(p0).__name__, e)

    time.sleep(2)

    # ========================================
    # Approach 4: Use pyservicemaker Pipeline's start_recording API
    # via direct access to internal pipeline
    # ========================================
    logger.info("--- Approach 4: pyservicemaker Pipeline.start_recording ---")
    try:
        from pyservicemaker import Pipeline as PsmPipeline
        # Try to use pyservicemaker to trigger recording
        # Build a pyservicemaker pipeline with same config
        psm_pipeline = PsmPipeline("sr-psm-test")
        psm_src = psm_pipeline.add("nvmultiurisrcbin", "src", {
            "ip-address": "0.0.0.0",
            "port": 0,  # disable REST
            "max-batch-size": 4,
            "smart-record": 2,
            "smart-rec-dir-path": RECORDING_DIR,
            "smart-rec-cache": 10,
            "smart-rec-default-duration": 15,
        })
        psm_pipeline.add("fakesink", "sink", {"sync": 0, "async": 0})
        psm_pipeline.link("src", "sink")

        # Check if start_recording exists
        logger.info("Pipeline methods: %s", [m for m in dir(psm_pipeline) if "record" in m.lower()])
        logger.info("Pipeline methods (all): %s", [m for m in dir(psm_pipeline) if not m.startswith("_")])
    except Exception as e:
        logger.warning("Approach 4 failed: %s", e)

    # ========================================
    # Approach 5: Check if nvmultiurisrcbin has a child
    # "record_bin" that we can manipulate directly
    # ========================================
    logger.info("--- Approach 5: Direct record bin manipulation ---")
    try:
        # Find record_bin0 inside the nvurisrcbin
        it = uribin.iterate_recurse()
        record_bins = []
        while True:
            ret, child = it.next()
            if ret == Gst.IteratorResult.OK:
                name = child.get_name()
                if "record" in name.lower():
                    record_bins.append(child)
                    logger.info("  Found: %s (type=%s, state=%s)",
                                name, type(child).__gtype__.name,
                                child.get_state(0)[1].value_nick)
            elif ret in (Gst.IteratorResult.DONE, Gst.IteratorResult.ERROR):
                break
            elif ret == Gst.IteratorResult.RESYNC:
                it.resync()

        if record_bins:
            # Check if any record bin has start/stop signals
            for rb in record_bins:
                gtype = GObject.type_from_name(type(rb).__gtype__.name)
                sigs = GObject.signal_list_ids(gtype)
                for sid in sigs:
                    info = GObject.signal_query(sid)
                    logger.info("  Signal on %s: %s", rb.get_name(), info.signal_name)
    except Exception as e:
        logger.warning("Approach 5 failed: %s", e)

    # Wait for any recording
    logger.info("\n--- Monitoring for 15 seconds ---")
    for t in range(15):
        time.sleep(1)
        try:
            sr = uribin.get_property("smart-rec-status")
        except Exception:
            sr = "?"
        files = []
        for root, dirs, fnames in os.walk(RECORDING_DIR):
            for f in fnames:
                full = os.path.join(root, f)
                files.append((f, os.path.getsize(full)))
        if sr or files:
            logger.info("t=%ds: sr_status=%s files=%s", t + 1, sr, files)

    logger.info("\n=== Final files ===")
    total = 0
    for root, dirs, fnames in os.walk(RECORDING_DIR):
        for f in fnames:
            full = os.path.join(root, f)
            sz = os.path.getsize(full)
            total += 1
            logger.info("  %s (%d bytes)", full, sz)

    if total == 0:
        logger.warning("NO recording files were produced.")
    else:
        logger.info("Total: %d files", total)

    # Cleanup
    payload["value"]["change"] = "camera_remove"
    requests.post(f"{REST_BASE}/api/v1/stream/remove", json=payload, timeout=10)
    time.sleep(2)
    pipeline.set_state(Gst.State.NULL)
    time.sleep(1)
    logger.info("Done.")


if __name__ == "__main__":
    run_test()
