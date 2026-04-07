#!/usr/bin/env python3
"""Probe v6: Final targeted test.

1. Check if pipeline["dsnvurisrcbin0"] works in pyservicemaker
2. Call start_recording AFTER stream data is flowing (wait for decoding)
3. Try passing source_id=0 (int) as source_name
"""

import logging
import os
import time

import requests

from pyservicemaker import (
    Pipeline, DynamicSourceMessage, StateTransitionMessage, PipelineState,
)

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger("probe-sr-v6")

REST_PORT = 9001
REST_BASE = f"http://127.0.0.1:{REST_PORT}"
CAMERA_URL = "rtsp://127.0.0.1:8555/video1"
CAMERA_ID = "probe_cam_v6"
RECORDING_DIR = "/tmp/sr_probe_recordings_v6"


def run_test():
    os.makedirs(RECORDING_DIR, exist_ok=True)
    for f in os.listdir(RECORDING_DIR):
        os.remove(os.path.join(RECORDING_DIR, f))

    pipeline = Pipeline("sr-v6")

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
        "select-rtp-protocol": 4,  # TCP only — avoid UDP timeout
        "latency": 100,
        "file-loop": 1,
        "smart-record": 2,
        "smart-rec-dir-path": RECORDING_DIR,
        "smart-rec-file-prefix": "v6_rec",
        "smart-rec-cache": 10,
        "smart-rec-container": 0,
        "smart-rec-mode": 1,
        "smart-rec-default-duration": 15,
    })

    pipeline.add("fakesink", "sink", {"sync": 0, "async": 0})
    pipeline.link("src", "sink")

    stream_flowing = {"value": False}
    source_info = {"source_id": -1}

    def on_message(message):
        if isinstance(message, DynamicSourceMessage):
            if message.source_added:
                source_info["source_id"] = message.source_id
                logger.info("Stream added: source_id=%d", message.source_id)
            else:
                logger.info("Stream removed: source_id=%d", message.source_id)
        elif isinstance(message, StateTransitionMessage):
            if "decoder" in message.origin and message.new_state == PipelineState.PLAYING:
                stream_flowing["value"] = True
                logger.info("Decoder %s -> PLAYING (stream data is flowing)", message.origin)

    pipeline.prepare(on_message)
    pipeline.activate()
    time.sleep(3)

    # Add stream
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_name": "Probe V6",
            "camera_url": CAMERA_URL,
            "change": "camera_add",
        },
    }
    r = requests.post(f"{REST_BASE}/api/v1/stream/add", json=payload, timeout=10)
    logger.info("Add stream: %d", r.status_code)

    # Wait for stream to be fully flowing (decoder playing)
    logger.info("Waiting for stream data to flow ...")
    for _ in range(30):
        if stream_flowing["value"]:
            break
        time.sleep(1)

    if not stream_flowing["value"]:
        logger.error("Stream not flowing after 30s!")
        pipeline.stop()
        return

    logger.info("Stream is flowing, waiting 3 more seconds for stability ...")
    time.sleep(3)

    # Test 1: Element lookup
    logger.info("\n=== Test 1: Element lookup ===")
    for name in ["src", "dsnvurisrcbin0", "src_creator", "sink"]:
        try:
            elem = pipeline[name]
            logger.info("  pipeline['%s'] = %s (type=%s)", name, elem, type(elem))
        except Exception as e:
            logger.warning("  pipeline['%s'] failed: %s", name, e)

    # Test 2: start_recording with various names, now that stream is flowing
    logger.info("\n=== Test 2: start_recording (stream is flowing) ===")

    def sr_callback(info):
        logger.info("SR CALLBACK! info=%s type=%s", info, type(info))
        for attr in dir(info):
            if not attr.startswith("_"):
                try:
                    logger.info("  %s = %s", attr, getattr(info, attr))
                except Exception:
                    pass

    test_names = [
        "dsnvurisrcbin0",
        "src",
        "src_creator/dsnvurisrcbin0",
    ]
    for name in test_names:
        logger.info("--- start_recording('%s', 0, 15) ---", name)
        try:
            session_id = pipeline.start_recording(name, 0, 15, sr_callback)
            logger.info("  result session_id = %d", session_id)
        except Exception as e:
            logger.warning("  failed: %s", e)

    # Wait for recording
    logger.info("\n--- Monitoring for 20 seconds ---")
    for t in range(20):
        time.sleep(1)
        files = []
        for root, dirs, fnames in os.walk(RECORDING_DIR):
            for f in fnames:
                full = os.path.join(root, f)
                files.append((f, os.path.getsize(full)))
        if files:
            logger.info("t=%ds: files=%s", t + 1, files)

    # Final check
    logger.info("\n=== Final recording directory ===")
    total = 0
    for root, dirs, fnames in os.walk(RECORDING_DIR):
        for f in fnames:
            full = os.path.join(root, f)
            sz = os.path.getsize(full)
            total += 1
            logger.info("  %s (%d bytes)", full, sz)

    if total:
        logger.info("SUCCESS! %d recording files.", total)
    else:
        logger.warning("NO recording files. SmartRecord through pyservicemaker DOES NOT WORK with nvmultiurisrcbin.")

    # Cleanup
    payload["value"]["change"] = "camera_remove"
    try:
        requests.post(f"{REST_BASE}/api/v1/stream/remove", json=payload, timeout=10)
    except Exception:
        pass
    time.sleep(2)
    pipeline.stop()
    logger.info("Done.")


if __name__ == "__main__":
    from multiprocessing import Process
    p = Process(target=run_test)
    p.start()
    p.join(timeout=120)
    if p.is_alive():
        p.terminate()
        p.join()
