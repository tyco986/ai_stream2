#!/usr/bin/env python3
"""Probe v3: Discover the exact start-sr signal signature and emit it correctly.

Uses a standalone pipeline on port 9001.
"""

import logging
import os
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
logger = logging.getLogger("probe-sr-v3")

REST_PORT = 9001
REST_BASE = f"http://127.0.0.1:{REST_PORT}"
CAMERA_URL = "rtsp://127.0.0.1:8555/video1"
CAMERA_ID = "probe_cam_sr3"
RECORDING_DIR = "/tmp/sr_probe_recordings"


def _find_nvurisrcbin(src_bin):
    """Find the child nvurisrcbin in nvmultiurisrcbin."""
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


def _query_signal_params(element, signal_name):
    """Query all parameter types for a given signal."""
    gtype = GObject.type_from_name(type(element).__gtype__.name)
    signal_ids = GObject.signal_list_ids(gtype)
    for sid in signal_ids:
        info = GObject.signal_query(sid)
        if info.signal_name == signal_name:
            param_types = info.param_types
            logger.info("Signal '%s' details:", signal_name)
            logger.info("  return type: %s", info.return_type.name)
            logger.info("  n params: %d", len(param_types))
            for i, pt in enumerate(param_types):
                logger.info("  param[%d]: %s (fundamental=%s)",
                           i, pt.name, pt.fundamental.name if hasattr(pt, 'fundamental') else "?")
            return info, param_types
    return None, None


def run_test():
    os.makedirs(RECORDING_DIR, exist_ok=True)

    logger.info("=== Building test pipeline ===")
    pipeline = Gst.Pipeline.new("sr-test-pipeline-v3")

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
    src.set_property("smart-rec-default-duration", 10)

    sink = Gst.ElementFactory.make("fakesink", "sink")
    sink.set_property("sync", 0)
    sink.set_property("async", 0)

    pipeline.add(src)
    pipeline.add(sink)
    src.link(sink)

    ret = pipeline.set_state(Gst.State.PLAYING)
    logger.info("Pipeline state: %s", ret)
    time.sleep(3)

    # Add stream
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_name": "Probe Camera V3",
            "camera_url": CAMERA_URL,
            "change": "camera_add",
        },
    }
    r = requests.post(f"{REST_BASE}/api/v1/stream/add", json=payload, timeout=10)
    logger.info("Stream add: %d", r.status_code)
    time.sleep(8)

    # Find child nvurisrcbin
    uribin = _find_nvurisrcbin(src)
    if not uribin:
        logger.error("No nvurisrcbin found!")
        pipeline.set_state(Gst.State.NULL)
        return

    logger.info("Found nvurisrcbin: %s", uribin.get_name())

    # Query signal parameters
    logger.info("\n=== start-sr signal signature ===")
    info, param_types = _query_signal_params(uribin, "start-sr")

    logger.info("\n=== stop-sr signal signature ===")
    _query_signal_params(uribin, "stop-sr")

    logger.info("\n=== sr-done signal signature ===")
    _query_signal_params(uribin, "sr-done")

    # Try to emit start-sr with 4 parameters based on discovered types
    if param_types and len(param_types) == 4:
        logger.info("\n=== Emitting start-sr with discovered types ===")
        type_names = [pt.name for pt in param_types]
        logger.info("Expected types: %s", type_names)

        # Common SmartRecord signal signatures in DeepStream:
        # start-sr(source_id: guint, start_time: guint, duration: guint, user_data: gpointer)
        # or
        # start-sr(session_id: guint, start_time: guint, duration: guint, user_data: gpointer)
        test_combos = [
            # (source_id, start_time, duration, user_data)
            (0, 0, 15, None),
            # (session_id, start_time, duration, user_data)
            (0, 0, 15, 0),
            # (start_time, duration, user_data, ?)
            (0, 15, None, None),
            # All zeros
            (0, 0, 0, None),
        ]

        for combo in test_combos:
            logger.info("Trying start-sr(%s) ...", combo)
            try:
                uribin.emit("start-sr", *combo)
                logger.info("  SUCCESS!")

                time.sleep(2)
                sr_status = uribin.get_property("smart-rec-status")
                logger.info("  smart-rec-status = %s", sr_status)

                if sr_status:
                    logger.info("  RECORDING STARTED!")
                    break
            except Exception as e:
                logger.warning("  Failed: %s", e)
    elif param_types:
        logger.info("\n=== Trying brute-force parameter combos ===")
        n = len(param_types)
        type_names = [pt.name for pt in param_types]
        logger.info("Need %d params of types: %s", n, type_names)

        # Build args based on type names
        args = []
        for pt in param_types:
            tname = pt.name.lower()
            if "uint" in tname or "int" in tname:
                args.append(0)
            elif "bool" in tname:
                args.append(True)
            elif "pointer" in tname or "boxed" in tname or tname == "void":
                args.append(None)
            elif "string" in tname or "char" in tname:
                args.append("")
            else:
                args.append(0)

        logger.info("Auto-typed args: %s", args)
        try:
            uribin.emit("start-sr", *args)
            logger.info("emit succeeded!")
        except Exception as e:
            logger.error("emit failed: %s", e)

        # Try with specific durations
        args_with_duration = list(args)
        for i, pt in enumerate(param_types):
            if "uint" in pt.name.lower():
                args_with_duration[i] = 15  # 15 seconds
                break
        logger.info("Args with duration: %s", args_with_duration)
        try:
            uribin.emit("start-sr", *args_with_duration)
            logger.info("emit with duration succeeded!")
        except Exception as e:
            logger.error("emit with duration failed: %s", e)

    # Wait and check for recordings
    logger.info("\nWaiting 15 seconds for recording to complete ...")
    for t in range(15):
        time.sleep(1)
        sr_status = uribin.get_property("smart-rec-status")
        files = []
        for root, dirs, fnames in os.walk(RECORDING_DIR):
            for f in fnames:
                full = os.path.join(root, f)
                files.append((full, os.path.getsize(full)))
        if files or sr_status:
            logger.info("  t=%ds: sr_status=%s files=%s", t + 1, sr_status, files)

    # Final file check
    logger.info("\n=== Final recording directory ===")
    for root, dirs, fnames in os.walk(RECORDING_DIR):
        for f in fnames:
            full = os.path.join(root, f)
            logger.info("  %s (%d bytes)", full, os.path.getsize(full))

    if not any(os.scandir(RECORDING_DIR)):
        logger.warning("No recording files produced!")

    # Cleanup
    logger.info("\n=== Cleanup ===")
    payload["value"]["change"] = "camera_remove"
    try:
        r = requests.post(f"{REST_BASE}/api/v1/stream/remove", json=payload, timeout=10)
        logger.info("Stream remove: %d", r.status_code)
    except Exception:
        pass
    time.sleep(2)
    pipeline.set_state(Gst.State.NULL)
    logger.info("Done.")


if __name__ == "__main__":
    run_test()
