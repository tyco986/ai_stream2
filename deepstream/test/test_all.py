#!/usr/bin/env python3
"""
Run all DeepStream API tests with continue-all strategy.

Example:
  python3 deepstream/test/test_all.py --base-url http://127.0.0.1:9000 --kafka-broker 127.0.0.1:9092
"""

import argparse
import subprocess
import sys
import time
from pathlib import Path

from _common import (
    fetch_stream_info,
    parse_source_id,
    prepare_camera,
    reset_streams,
)


TEST_ORDER = [
    "test_health_get_dsready_state.py",
    "test_stream_get_stream_info.py",
    "test_stream_add.py",
    "test_command_start_rolling.py",
    "test_command_stop_rolling.py",
    "test_command_start_recording_event.py",
    "test_command_start_recording_manual.py",
    "test_command_stop_recording.py",
    "test_command_screenshot.py",
    "test_command_switch_preview.py",
    "test_stream_remove.py",
]

COMMAND_TESTS = {
    "test_command_start_rolling.py",
    "test_command_stop_rolling.py",
    "test_command_start_recording_event.py",
    "test_command_start_recording_manual.py",
    "test_command_stop_recording.py",
    "test_command_screenshot.py",
    "test_command_switch_preview.py",
}


def parse_args():
    parser = argparse.ArgumentParser(description="Run all DeepStream API tests")
    parser.add_argument("--base-url", default="http://127.0.0.1:9000")
    parser.add_argument("--kafka-broker", default="127.0.0.1:9092")
    parser.add_argument("--command-topic", default="deepstream-commands")
    parser.add_argument("--camera-url", default="file:///app/example_data/video2_bf0.mp4")
    parser.add_argument("--camera-id", default="test_camera")
    parser.add_argument("--camera-name", default="Test Camera")
    parser.add_argument("--timeout", type=int, default=15)
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--python", default=sys.executable, help="Python interpreter to run test scripts")
    return parser.parse_args()


def main():
    args = parse_args()
    test_dir = Path(__file__).resolve().parent
    started_at = time.time()
    results = []
    persistent_source_id = -1

    print("\n=== PREPARE PERSISTENT CAMERA ===")
    try:
        prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)
        _, info_data = fetch_stream_info(args.base_url, args.timeout)
        persistent_source_id = parse_source_id(info_data, args.camera_id)
        print(f"Prepared source_id={persistent_source_id}")
    except Exception as exc:
        print(f"Prepare failed: {exc}")
        persistent_source_id = -1

    for script_name in TEST_ORDER:
        script_path = test_dir / script_name
        cmd = [
            args.python,
            str(script_path),
            "--base-url",
            args.base_url,
            "--camera-id",
            args.camera_id,
            "--camera-name",
            args.camera_name,
            "--camera-url",
            args.camera_url,
            "--timeout",
            str(args.timeout),
        ]
        if script_name in COMMAND_TESTS:
            cmd.extend(
                [
                    "--kafka-broker",
                    args.kafka_broker,
                    "--command-topic",
                    args.command_topic,
                ]
            )
        if script_name in COMMAND_TESTS and persistent_source_id != -1:
            cmd.append("--no-prepare")
        if args.verbose:
            cmd.append("--verbose")

        case_started = time.time()
        print(f"\n=== RUN {script_name} ===")
        completed = subprocess.run(cmd, check=False)
        elapsed = time.time() - case_started
        passed = completed.returncode == 0
        results.append((script_name, passed, completed.returncode, elapsed))
        print(f"=== {'PASS' if passed else 'FAIL'} {script_name} ({elapsed:.2f}s) ===")

    total_elapsed = time.time() - started_at
    passed_count = sum(1 for _, ok, _, _ in results if ok)
    failed_cases = [(name, code) for name, ok, code, _ in results if not ok]

    print("\n========== SUMMARY ==========")
    print(f"Total: {len(results)}")
    print(f"Passed: {passed_count}")
    print(f"Failed: {len(failed_cases)}")
    print(f"Elapsed: {total_elapsed:.2f}s")

    if failed_cases:
        print("\nFailed cases:")
        for name, code in failed_cases:
            print(f"- {name}: exit_code={code}")
        exit_code = 1
    else:
        print("\nAll tests passed.")
        exit_code = 0

    print("\n=== CLEANUP PERSISTENT CAMERA ===")
    try:
        reset_streams(args.base_url, args.timeout, args.camera_url)
        print("Cleanup done.")
    except Exception as exc:
        print(f"Cleanup failed: {exc}")
        if exit_code == 0:
            exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
