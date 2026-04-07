#!/usr/bin/env python3
"""Probe v5: Test pyservicemaker Pipeline.start_recording() with nvmultiurisrcbin.

This script builds a full pyservicemaker Pipeline (not raw gi),
enables SmartRecord, adds a stream, and calls start_recording().
"""

import logging
import os
import time

import requests

from pyservicemaker import Pipeline, DynamicSourceMessage, StateTransitionMessage, PipelineState

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger("probe-sr-v5")

REST_PORT = 9001
REST_BASE = f"http://127.0.0.1:{REST_PORT}"
CAMERA_URL = "rtsp://127.0.0.1:8555/video1"
CAMERA_ID = "probe_cam_v5"
RECORDING_DIR = "/tmp/sr_probe_recordings_v5"


def run_test():
    os.makedirs(RECORDING_DIR, exist_ok=True)
    for f in os.listdir(RECORDING_DIR):
        os.remove(os.path.join(RECORDING_DIR, f))

    logger.info("=== Inspecting Pipeline API ===")
    # Check start_recording method signature
    import inspect
    try:
        sig = inspect.signature(Pipeline.start_recording)
        logger.info("start_recording signature: %s", sig)
    except Exception as e:
        logger.info("Cannot inspect signature: %s", e)

    try:
        doc = Pipeline.start_recording.__doc__
        logger.info("start_recording doc: %s", doc)
    except Exception as e:
        logger.info("No docstring: %s", e)

    try:
        sig = inspect.signature(Pipeline.stop_recording)
        logger.info("stop_recording signature: %s", sig)
        doc = Pipeline.stop_recording.__doc__
        logger.info("stop_recording doc: %s", doc)
    except Exception as e:
        logger.info("Cannot inspect stop_recording: %s", e)

    try:
        sig = inspect.signature(Pipeline.stop_recording_by_session_id)
        logger.info("stop_recording_by_session_id signature: %s", sig)
        doc = Pipeline.stop_recording_by_session_id.__doc__
        logger.info("stop_recording_by_session_id doc: %s", doc)
    except Exception as e:
        logger.info("Cannot inspect stop_recording_by_session_id: %s", e)

    # Also check help
    try:
        logger.info("\n=== help(Pipeline.start_recording) ===")
        help(Pipeline.start_recording)
    except Exception as e:
        logger.info("help failed: %s", e)

    # Build pipeline
    logger.info("\n=== Building pyservicemaker pipeline ===")
    pipeline = Pipeline("sr-psm-test")

    pipeline.add("nvmultiurisrcbin", "src", {
        "ip-address": "0.0.0.0",
        "port": REST_PORT,
        "max-batch-size": 4,
        "batched-push-timeout": 33333,
        "width": 1920,
        "height": 1080,
        "live-source": 1,
        "drop-pipeline-eos": 1,
        "async-handling": 1,
        "select-rtp-protocol": 0,
        "latency": 100,
        "file-loop": 1,
        "smart-record": 2,
        "smart-rec-dir-path": RECORDING_DIR,
        "smart-rec-file-prefix": "psm_rec",
        "smart-rec-cache": 10,
        "smart-rec-container": 0,
        "smart-rec-mode": 1,
        "smart-rec-default-duration": 15,
    })

    pipeline.add("fakesink", "sink", {"sync": 0, "async": 0})
    pipeline.link("src", "sink")

    # Track source state
    source_ready = {"ready": False, "source_id": -1}

    def on_message(message):
        if isinstance(message, DynamicSourceMessage):
            if message.source_added:
                source_ready["ready"] = True
                source_ready["source_id"] = message.source_id
                logger.info("Stream added: sensor_id=%s source_id=%d",
                            message.sensor_id, message.source_id)
            else:
                source_ready["ready"] = False
                logger.info("Stream removed: source_id=%d", message.source_id)
        elif isinstance(message, StateTransitionMessage):
            if message.new_state == PipelineState.PLAYING:
                logger.info("Element %s -> PLAYING", message.origin)

    pipeline.prepare(on_message)
    pipeline.activate()
    logger.info("Pipeline activated, waiting 3 seconds ...")
    time.sleep(3)

    # Add a stream
    logger.info("=== Adding stream ===")
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_name": "Probe Camera V5",
            "camera_url": CAMERA_URL,
            "change": "camera_add",
        },
    }
    r = requests.post(f"{REST_BASE}/api/v1/stream/add", json=payload, timeout=10)
    logger.info("Add stream: %d", r.status_code)

    # Wait for stream to be ready
    for _ in range(20):
        if source_ready["ready"]:
            break
        time.sleep(0.5)

    if not source_ready["ready"]:
        logger.error("Stream not ready after 10s!")
        pipeline.deactivate()
        return

    source_id = source_ready["source_id"]
    logger.info("Stream ready, source_id=%d", source_id)
    time.sleep(3)

    # ========================================
    # Test 1: start_recording(source_id)
    # ========================================
    logger.info("\n=== Test 1: start_recording(source_id=%d) ===", source_id)
    try:
        result = pipeline.start_recording(source_id)
        logger.info("start_recording result: %s (type=%s)", result, type(result))
    except TypeError as e:
        logger.warning("TypeError: %s", e)
        # Try with different param combos
        logger.info("Trying various parameter combinations:")
        test_args = [
            (source_id,),
            (source_id, 15),
            (source_id, 0, 15),
            ("dsnvurisrcbin0",),
            ("dsnvurisrcbin0", 15),
            ("dsnvurisrcbin0", 0, 15),
            ("src", source_id),
            ("src", source_id, 15),
        ]
        for args in test_args:
            try:
                result = pipeline.start_recording(*args)
                logger.info("  start_recording(%s) = %s", args, result)
            except Exception as ex:
                logger.warning("  start_recording(%s) failed: %s", args, ex)
    except Exception as e:
        logger.error("start_recording error: %s (%s)", e, type(e).__name__)

    # Wait for recording
    logger.info("\n--- Monitoring for 15 seconds ---")
    for t in range(15):
        time.sleep(1)
        files = []
        for root, dirs, fnames in os.walk(RECORDING_DIR):
            for f in fnames:
                full = os.path.join(root, f)
                files.append((f, os.path.getsize(full)))
        if files:
            logger.info("t=%ds: files=%s", t + 1, files)

    logger.info("\n=== Final recording directory ===")
    total = 0
    for root, dirs, fnames in os.walk(RECORDING_DIR):
        for f in fnames:
            full = os.path.join(root, f)
            sz = os.path.getsize(full)
            total += 1
            logger.info("  %s (%d bytes)", full, sz)

    if total == 0:
        logger.warning("NO recording files produced.")
    else:
        logger.info("SUCCESS! %d files produced.", total)

    # Try stop_recording
    logger.info("\n=== Testing stop_recording ===")
    try:
        result = pipeline.stop_recording(source_id)
        logger.info("stop_recording result: %s", result)
    except Exception as e:
        logger.warning("stop_recording failed: %s", e)

    # Cleanup
    logger.info("\n=== Cleanup ===")
    payload["value"]["change"] = "camera_remove"
    try:
        requests.post(f"{REST_BASE}/api/v1/stream/remove", json=payload, timeout=10)
    except Exception:
        pass
    time.sleep(2)
    pipeline.deactivate()
    logger.info("Done.")


if __name__ == "__main__":
    from multiprocessing import Process
    p = Process(target=run_test)
    p.start()
    p.join(timeout=120)
    if p.is_alive():
        p.terminate()
        p.join()
