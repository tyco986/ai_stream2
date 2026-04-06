#!/usr/bin/env python3
"""
Test command action: screenshot

Example:
  python3 deepstream/test/test_command_screenshot.py \
    --base-url http://127.0.0.1:9000 --kafka-broker 127.0.0.1:9092
"""

import subprocess
import time

from _common import (
    build_producer,
    create_parser,
    get_screenshots_dir,
    prepare_camera,
    send_command,
    wait_for_file,
    write_test_payload,
)


def _wait_for_file_in_container(container: str, path: str, wait_seconds: int) -> bool:
    """Poll for file existence inside a Docker container."""
    deadline = time.time() + wait_seconds
    while time.time() < deadline:
        result = subprocess.run(
            ["docker", "exec", container, "test", "-s", path],
            capture_output=True, check=False,
        )
        if result.returncode == 0:
            return True
        time.sleep(1)
    return False


def main():
    parser = create_parser("Test DeepStream command screenshot", include_kafka=True)
    parser.add_argument("--wait-seconds", type=int, default=20, help="Wait for screenshot file creation")
    parser.add_argument("--no-prepare", action="store_true", help="Use pre-existing camera source")
    parser.add_argument("--docker-container", default="", help="Check file inside this container instead of host")
    args = parser.parse_args()

    if not args.no_prepare:
        prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)

    screenshot_dir = get_screenshots_dir()
    screenshot_dir.mkdir(parents=True, exist_ok=True)
    filename = f"{args.camera_id}_{int(time.time())}.jpg"

    payload = {"action": "screenshot", "source_id": args.camera_id, "filename": filename}
    write_test_payload(f"{args.camera_id}_command_screenshot.json", payload)

    producer = build_producer(args.kafka_broker)
    send_command(producer, args.command_topic, payload, args.timeout)

    if args.docker_container:
        container_path = f"/app/screenshots/{filename}"
        found = _wait_for_file_in_container(args.docker_container, container_path, args.wait_seconds)
        display_path = f"{args.docker_container}:{container_path}"
    else:
        expected_file = screenshot_dir / filename
        found = wait_for_file(expected_file, wait_seconds=args.wait_seconds)
        display_path = str(expected_file)

    if not found:
        raise AssertionError(f"Screenshot file not found: {display_path}")

    print(f"PASS: command screenshot sensor_id={args.camera_id} file={display_path}")


if __name__ == "__main__":
    main()
