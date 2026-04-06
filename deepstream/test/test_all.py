#!/usr/bin/env python3
"""
Run all DeepStream API tests with continue-all strategy.

Example (inside container):
  python3 deepstream/test/test_all.py --base-url http://127.0.0.1:9000 --kafka-broker 127.0.0.1:9092

Example (from host, with auto-restart):
  python3 deepstream/test/test_all.py --base-url http://127.0.0.1:9000 \
      --kafka-broker 127.0.0.1:19092 --docker-container ai-stream2-deepstream
"""

import argparse
import subprocess
import sys
import time
from pathlib import Path

from _common import (
    ensure_camera_added,
    fetch_stream_info,
    find_source_id_by_camera_id,
    http_get_json,
    reset_streams,
    wait_for_source_id,
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

NEEDS_PREPARE = {
    "test_stream_add.py",
    "test_stream_remove.py",
    *COMMAND_TESTS,
}


class ContainerManager:
    """Restart a Docker container and wait for the DeepStream REST health
    endpoint.  Used when running tests from the host to survive pipeline
    exits caused by removing the last file-based source."""

    def __init__(self, container_name, base_url, timeout, health_wait=60):
        self._name = container_name
        self._base_url = base_url
        self._timeout = timeout
        self._health_wait = health_wait

    def _is_running(self):
        result = subprocess.run(
            ["docker", "ps", "-q", "--filter", f"name={self._name}", "--filter", "status=running"],
            capture_output=True, text=True, check=False,
        )
        return bool(result.stdout.strip())

    def _start(self):
        subprocess.run(["docker", "start", self._name], capture_output=True, check=False)

    def _wait_healthy(self):
        deadline = time.time() + self._health_wait
        while time.time() < deadline:
            try:
                resp, _ = http_get_json(self._base_url, "/api/v1/health/get-dsready-state", self._timeout)
                if resp.status_code == 200:
                    return True
            except Exception:
                pass
            time.sleep(2)
        return False

    def ensure_running(self):
        if self._is_running():
            return True
        print(f"  [container] {self._name} not running, restarting …")
        self._start()
        if self._wait_healthy():
            print(f"  [container] {self._name} is ready")
            return True
        print(f"  [container] {self._name} failed to become ready")
        return False


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
    parser.add_argument(
        "--docker-container",
        default="",
        help="Docker container name; when set, auto-restart the container if it exits between tests",
    )
    return parser.parse_args()


def _ensure_pipeline_ready(container_mgr, base_url, timeout, camera_id, camera_name, camera_url):
    """Ensure the container is running and the persistent camera is added.

    Unlike ``prepare_camera`` this does NOT call ``reset_streams`` first,
    because removing all file-based sources causes the DeepStream pipeline
    to exit.  Instead we check if the camera already exists and only add
    it when missing.  On any connection failure we restart the container
    and retry once.
    """
    max_attempts = 2 if container_mgr else 1
    for attempt in range(max_attempts):
        if container_mgr:
            container_mgr.ensure_running()
        try:
            _, info_data = fetch_stream_info(base_url, timeout)
            source_id = find_source_id_by_camera_id(info_data, camera_id)
            if source_id is not None:
                return source_id
            ensure_camera_added(base_url, timeout, camera_id, camera_name, camera_url)
            return wait_for_source_id(base_url, timeout, camera_id)
        except Exception:
            if attempt < max_attempts - 1:
                print(f"  [container] connection lost, restarting …")
                if container_mgr:
                    # Force a restart by stopping first
                    subprocess.run(
                        ["docker", "restart", container_mgr._name],
                        capture_output=True, check=False,
                    )
                    container_mgr._wait_healthy()
                continue
            raise


def main():
    args = parse_args()
    test_dir = Path(__file__).resolve().parent
    started_at = time.time()
    results = []

    container_mgr = ContainerManager(args.docker_container, args.base_url, args.timeout) if args.docker_container else None

    print("\n=== PREPARE PERSISTENT CAMERA ===")
    try:
        persistent_source_id = _ensure_pipeline_ready(
            container_mgr, args.base_url, args.timeout,
            args.camera_id, args.camera_name, args.camera_url,
        )
        print(f"Prepared source_id={persistent_source_id}")
    except Exception as exc:
        print(f"Prepare failed: {exc}")
        persistent_source_id = -1

    for script_name in TEST_ORDER:
        # Before each test that touches streams, guarantee the container is alive
        # and the persistent camera exists.
        if container_mgr and script_name in NEEDS_PREPARE:
            try:
                persistent_source_id = _ensure_pipeline_ready(
                    container_mgr, args.base_url, args.timeout,
                    args.camera_id, args.camera_name, args.camera_url,
                )
            except Exception as exc:
                print(f"  [container] re-prepare failed: {exc}")
                persistent_source_id = -1

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
        if script_name in NEEDS_PREPARE and persistent_source_id != -1:
            cmd.append("--no-prepare")
        if args.docker_container:
            if script_name in ("test_command_screenshot.py", "test_stream_remove.py"):
                cmd.extend(["--docker-container", args.docker_container])
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

    print("\n=== CLEANUP ===")
    if container_mgr:
        container_mgr.ensure_running()
    try:
        reset_streams(args.base_url, args.timeout, args.camera_url)
        print("Cleanup done.")
    except Exception as exc:
        print(f"Cleanup failed (non-fatal): {exc}")

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
