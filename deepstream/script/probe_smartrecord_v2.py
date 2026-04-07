#!/usr/bin/env python3
"""Build a standalone test pipeline with nvmultiurisrcbin,
enable SmartRecord, add a stream, and inspect child elements.

Must be run while the RTSP test server is running on port 8555.
Uses a separate REST port (9001) to avoid conflicting with the main pipeline.
"""

import json
import logging
import os
import threading
import time

import requests

import gi
gi.require_version("Gst", "1.0")
from gi.repository import Gst, GLib, GObject

Gst.init(None)

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger("probe-sr")

REST_PORT = 9001
REST_BASE = f"http://127.0.0.1:{REST_PORT}"
CAMERA_URL = "rtsp://127.0.0.1:8555/video1"
CAMERA_ID = "probe_cam_sr"
RECORDING_DIR = "/tmp/sr_probe_recordings"


def _recursive_children(element, depth=0):
    """Recursively list all children of a GstBin."""
    results = []
    if not isinstance(element, Gst.Bin):
        return results
    it = element.iterate_elements()
    while True:
        ret, child = it.next()
        if ret == Gst.IteratorResult.OK:
            indent = "  " * depth
            type_name = child.__class__.__gtype__.name
            factory = child.get_factory()
            factory_name = factory.get_name() if factory else "(no factory)"
            results.append((depth, child.get_name(), type_name, factory_name, child))
            logger.info("%s child: name=%s type=%s factory=%s",
                        indent, child.get_name(), type_name, factory_name)
            results.extend(_recursive_children(child, depth + 1))
        elif ret in (Gst.IteratorResult.DONE, Gst.IteratorResult.ERROR):
            break
        elif ret == Gst.IteratorResult.RESYNC:
            it.resync()
    return results


def _list_signals(element):
    """List all signals on a GstElement."""
    gtype = GObject.type_from_name(type(element).__gtype__.name)
    signal_ids = GObject.signal_list_ids(gtype)
    for sid in signal_ids:
        info = GObject.signal_query(sid)
        flags_str = str(info.signal_flags)
        is_action = bool(info.signal_flags & 32)
        logger.info("  signal: %s (action=%s, flags=%s, return=%s)",
                     info.signal_name, is_action, flags_str, info.return_type.name)
    return signal_ids


def _list_sr_properties(element):
    """List SmartRecord-related properties."""
    props = GObject.list_properties(element)
    sr_props = {}
    for prop in props:
        name = prop.name
        if any(kw in name.lower() for kw in ("smart", "record", "sr")):
            try:
                val = element.get_property(name)
            except Exception as e:
                val = f"<error: {e}>"
            sr_props[name] = val
            logger.info("  %s = %s", name, val)
    return sr_props


def run_test():
    os.makedirs(RECORDING_DIR, exist_ok=True)

    logger.info("=== Building test pipeline ===")
    pipeline = Gst.Pipeline.new("sr-test-pipeline")

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

    # SmartRecord configuration on nvmultiurisrcbin
    src.set_property("smart-record", 2)  # 2 = enable with cloud events
    src.set_property("smart-rec-dir-path", RECORDING_DIR)
    src.set_property("smart-rec-file-prefix", "probe_rec")
    src.set_property("smart-rec-video-cache", 10)
    src.set_property("smart-rec-cache", 10)
    src.set_property("smart-rec-container", 0)  # MP4
    src.set_property("smart-rec-mode", 1)  # video only
    src.set_property("smart-rec-default-duration", 10)

    sink = Gst.ElementFactory.make("fakesink", "sink")
    sink.set_property("sync", 0)
    sink.set_property("async", 0)

    pipeline.add(src)
    pipeline.add(sink)
    src.link(sink)

    logger.info("SmartRecord properties on nvmultiurisrcbin after config:")
    _list_sr_properties(src)

    # Start pipeline
    logger.info("=== Setting pipeline to PLAYING ===")
    ret = pipeline.set_state(Gst.State.PLAYING)
    logger.info("set_state result: %s", ret)

    time.sleep(3)

    # Add a stream
    logger.info("=== Adding stream via REST ===")
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_name": "Probe SR Camera",
            "camera_url": CAMERA_URL,
            "change": "camera_add",
        },
    }
    try:
        r = requests.post(f"{REST_BASE}/api/v1/stream/add", json=payload, timeout=10)
        logger.info("POST /stream/add status=%d body=%s", r.status_code, r.text[:200])
    except Exception as e:
        logger.error("Failed to add stream: %s", e)
        pipeline.set_state(Gst.State.NULL)
        return

    # Wait for the stream to establish
    logger.info("Waiting 8 seconds for stream to connect ...")
    time.sleep(8)

    # ========================================
    # INTROSPECT: enumerate child elements
    # ========================================
    logger.info("=== Enumerating children of nvmultiurisrcbin ===")
    children = _recursive_children(src)

    nvurisrcbin_elements = [
        (name, child) for (_, name, gtype, factory, child) in children
        if factory == "nvurisrcbin" or "nvurisrcbin" in factory.lower()
    ]

    if not nvurisrcbin_elements:
        logger.warning("No nvurisrcbin child elements found. Looking by type name ...")
        nvurisrcbin_elements = [
            (name, child) for (_, name, gtype, factory, child) in children
            if "urisrcbin" in gtype.lower() or "urisrcbin" in name.lower()
        ]

    if not nvurisrcbin_elements:
        logger.error("Could not find any nvurisrcbin child elements!")
        logger.info("All children: %s", [(n, t, f) for (_, n, t, f, _) in children])
    else:
        logger.info("Found %d nvurisrcbin children:", len(nvurisrcbin_elements))
        for name, child in nvurisrcbin_elements:
            logger.info("  name=%s type=%s", name, type(child).__gtype__.name)

    # ========================================
    # INTROSPECT: signals on child nvurisrcbin
    # ========================================
    for name, child in nvurisrcbin_elements:
        logger.info("=== Signals on child '%s' ===", name)
        _list_signals(child)

        logger.info("=== SmartRecord properties on child '%s' ===", name)
        sr_props = _list_sr_properties(child)

        # Try to emit start-sr signal
        logger.info("=== Attempting to emit 'start-sr' on '%s' ===", name)
        try:
            child.emit("start-sr", 0, 15)  # source_id=0, duration=15s
            logger.info("start-sr emitted successfully!")
        except Exception as e:
            logger.error("Failed to emit start-sr: %s", e)

        # Try with different argument combinations
        logger.info("=== Trying alternative start-sr signatures ===")
        for args in [
            (0, 15),
            (15,),
            (),
        ]:
            try:
                child.emit("start-sr", *args)
                logger.info("start-sr(%s) succeeded!", args)
                break
            except Exception as e:
                logger.warning("start-sr(%s) failed: %s", args, e)

    # Wait for recording to start
    logger.info("Waiting 5 seconds for recording ...")
    time.sleep(5)

    # Check if any recording files were created
    logger.info("=== Checking recording directory ===")
    for root, dirs, files in os.walk(RECORDING_DIR):
        for f in files:
            full = os.path.join(root, f)
            size = os.path.getsize(full)
            logger.info("  FILE: %s (%d bytes)", full, size)

    if not os.listdir(RECORDING_DIR):
        logger.warning("No files in recording directory!")

    # Check SmartRecord status on child elements
    for name, child in nvurisrcbin_elements:
        try:
            sr_status = child.get_property("smart-rec-status")
            logger.info("smart-rec-status on '%s' = %s", name, sr_status)
        except Exception as e:
            logger.warning("Cannot read smart-rec-status on '%s': %s", name, e)

    # Try to stop recording
    for name, child in nvurisrcbin_elements:
        logger.info("=== Attempting to emit 'stop-sr' on '%s' ===", name)
        try:
            child.emit("stop-sr", 0)
            logger.info("stop-sr emitted successfully!")
        except Exception as e:
            logger.error("Failed to emit stop-sr: %s", e)

    time.sleep(3)

    # Final check of files
    logger.info("=== Final recording directory check ===")
    for root, dirs, files in os.walk(RECORDING_DIR):
        for f in files:
            full = os.path.join(root, f)
            size = os.path.getsize(full)
            logger.info("  FILE: %s (%d bytes)", full, size)

    # Cleanup
    logger.info("=== Cleanup: removing stream ===")
    try:
        payload["value"]["change"] = "camera_remove"
        r = requests.post(f"{REST_BASE}/api/v1/stream/remove", json=payload, timeout=10)
        logger.info("POST /stream/remove status=%d", r.status_code)
    except Exception as e:
        logger.warning("Failed to remove stream: %s", e)

    time.sleep(2)

    logger.info("=== Setting pipeline to NULL ===")
    pipeline.set_state(Gst.State.NULL)
    logger.info("=== Done ===")


if __name__ == "__main__":
    run_test()
