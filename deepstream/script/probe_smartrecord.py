#!/usr/bin/env python3
"""Probe nvmultiurisrcbin for SmartRecord capabilities.

Run inside the DeepStream container AFTER the pipeline is running
and at least one stream has been added.
"""

import json
import sys
import time

import requests

import gi
gi.require_version("Gst", "1.0")
from gi.repository import Gst, GObject  # noqa: E402

Gst.init(None)

DS_REST = "http://127.0.0.1:9000"
CAMERA_URL = "rtsp://127.0.0.1:8555/video1"
CAMERA_ID = "probe_camera"


def add_stream():
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_name": "Probe Camera",
            "camera_url": CAMERA_URL,
            "change": "camera_add",
        },
    }
    r = requests.post(f"{DS_REST}/api/v1/stream/add", json=payload, timeout=10)
    print(f"[add_stream] status={r.status_code}")
    return r.status_code in (200, 201)


def remove_stream():
    payload = {
        "key": "sensor",
        "value": {
            "camera_id": CAMERA_ID,
            "camera_url": CAMERA_URL,
            "change": "camera_remove",
        },
    }
    r = requests.post(f"{DS_REST}/api/v1/stream/remove", json=payload, timeout=10)
    print(f"[remove_stream] status={r.status_code}")


def get_stream_info():
    r = requests.get(f"{DS_REST}/api/v1/stream/get-stream-info", timeout=10)
    return r.json()


def find_pipeline():
    """Find the running GStreamer pipeline via the default context."""
    registry = Gst.Registry.get()
    # Iterate all elements in the default main context
    # We need to find the pipeline by iterating GstBin children
    # Since we're in a different process, we can't access the pipeline directly.
    # Instead, we'll use gst-inspect style introspection on the element factory.
    print("\n=== GStreamer Element Factory Inspection ===")

    factory = Gst.ElementFactory.find("nvmultiurisrcbin")
    if not factory:
        print("ERROR: nvmultiurisrcbin factory not found")
        return

    print(f"Factory: {factory.get_name()}")
    print(f"Long name: {factory.get_metadata('long-name')}")
    print(f"Description: {factory.get_metadata('description')}")
    print(f"Author: {factory.get_metadata('author')}")

    # Create a temporary element to inspect properties and signals
    elem = Gst.ElementFactory.make("nvmultiurisrcbin", "probe_elem")
    if not elem:
        print("ERROR: Could not create nvmultiurisrcbin element")
        return

    inspect_element(elem)


def inspect_element(elem):
    """Inspect properties and signals of a GStreamer element."""
    elem_type = type(elem)
    gtype = GObject.type_from_name(elem_type.__gtype__.name)

    # --- Properties ---
    print(f"\n=== Properties of {elem_type.__gtype__.name} ===")
    sr_props = []
    all_props = GObject.list_properties(elem)
    for prop in all_props:
        name = prop.name
        if "smart" in name.lower() or "record" in name.lower() or "sr" in name.lower():
            sr_props.append(prop)
            try:
                val = elem.get_property(name)
            except Exception as e:
                val = f"<error: {e}>"
            print(f"  {name} = {val}  (type={prop.value_type.name}, flags={prop.flags})")

    if not sr_props:
        print("  (no smart-record related properties found)")

    # --- Signals ---
    print(f"\n=== Signals of {elem_type.__gtype__.name} ===")
    signal_ids = GObject.signal_list_ids(gtype)
    sr_signals = []
    for sig_id in signal_ids:
        info = GObject.signal_query(sig_id)
        name = info.signal_name
        sr_signals.append(name)
        print(f"  signal: {name} (flags={info.signal_flags}, return_type={info.return_type.name})")

    if not sr_signals:
        print("  (no signals found)")

    # --- Action Signals ---
    print(f"\n=== Action Signals (callable) ===")
    for sig_id in signal_ids:
        info = GObject.signal_query(sig_id)
        # GObject.SignalFlags.ACTION = 32
        if info.signal_flags & 32:
            print(f"  ACTION SIGNAL: {info.signal_name} (params={info.n_params}, return={info.return_type.name})")

    # --- Also check parent types ---
    print(f"\n=== Checking parent type hierarchy ===")
    current = gtype
    while current and current.name != "GObject":
        print(f"  Type: {current.name}")
        parent = GObject.type_parent(current)
        if parent == current:
            break
        current = parent


def inspect_nvurisrcbin():
    """Also inspect nvurisrcbin for comparison."""
    elem = Gst.ElementFactory.make("nvurisrcbin", "probe_urisrc")
    if not elem:
        print("\nERROR: Could not create nvurisrcbin element")
        return

    print("\n\n========================================")
    print("=== nvurisrcbin inspection (for comparison) ===")
    print("========================================")
    inspect_element(elem)


def scan_rest_api():
    """Try various REST API paths to find recording endpoints."""
    print("\n=== Scanning REST API endpoints ===")
    paths = [
        "/api/v1/stream/start-recording",
        "/api/v1/stream/stop-recording",
        "/api/v1/stream/recording",
        "/api/v1/recording/start",
        "/api/v1/recording/stop",
        "/api/v1/smartrecord/start",
        "/api/v1/smartrecord/stop",
        "/api/v1/sr/start",
        "/api/v1/sr/stop",
        "/",
        "/api",
        "/api/v1",
    ]
    for path in paths:
        for method in ("GET", "POST"):
            try:
                r = requests.request(method, f"{DS_REST}{path}", timeout=3,
                                     json={"source_id": 0, "duration": 10})
                print(f"  {method} {path} → {r.status_code} ({len(r.text)} bytes)")
                if r.status_code == 200 and r.text.strip():
                    print(f"    body: {r.text[:200]}")
            except requests.exceptions.RequestException as e:
                print(f"  {method} {path} → ERROR: {e}")


def main():
    print("=" * 60)
    print("SmartRecord Diagnostic Probe")
    print("=" * 60)

    # Step 1: Add a stream so nvurisrcbin children exist
    print("\n--- Adding test stream ---")
    if not add_stream():
        print("WARNING: Failed to add stream, continuing anyway")

    time.sleep(5)

    info = get_stream_info()
    print(f"Stream info: {json.dumps(info, indent=2)}")

    # Step 2: Inspect element factories
    find_pipeline()

    # Step 3: Also inspect nvurisrcbin for comparison
    inspect_nvurisrcbin()

    # Step 4: Scan REST API
    scan_rest_api()

    # Step 5: Cleanup
    print("\n--- Removing test stream ---")
    remove_stream()

    print("\n=== Done ===")


if __name__ == "__main__":
    main()
