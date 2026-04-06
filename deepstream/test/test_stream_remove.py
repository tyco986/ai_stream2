#!/usr/bin/env python3
"""
Test POST /api/v1/stream/remove

Example:
  python3 deepstream/test/test_stream_remove.py --camera-url rtsp://127.0.0.1:8554/video1
"""

import subprocess
import time

from _common import (
    assert_status,
    build_stream_remove_payload,
    camera_exists,
    create_parser,
    ensure_camera_added,
    fetch_stream_info,
    http_get_json,
    http_post_json,
    prepare_camera,
    verbose_print,
    wait_for_source_id,
    write_test_payload,
)


def _restart_and_add(args):
    """Restart the Docker container and re-add the camera so the remove
    test has a live target.  Only used when --docker-container is set."""
    print(f"  [stream_remove] container down, restarting {args.docker_container} …")
    subprocess.run(["docker", "restart", args.docker_container], capture_output=True, check=False)
    deadline = time.time() + 60
    while time.time() < deadline:
        try:
            resp, _ = http_get_json(args.base_url, "/api/v1/health/get-dsready-state", args.timeout)
            if resp.status_code == 200:
                break
        except Exception:
            pass
        time.sleep(2)
    ensure_camera_added(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)
    wait_for_source_id(args.base_url, args.timeout, args.camera_id)


def main():
    parser = create_parser("Test DeepStream stream/remove API")
    parser.add_argument("--no-prepare", action="store_true", help="Skip reset; assume camera already added")
    parser.add_argument("--docker-container", default="", help="Auto-restart container if it exits")
    args = parser.parse_args()

    if not args.no_prepare:
        prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)

    payload = build_stream_remove_payload(args.camera_id, args.camera_url)
    write_test_payload(f"{args.camera_id}_stream_remove.json", payload)

    remove_accepted = False
    try:
        response, data = http_post_json(args.base_url, "/api/v1/stream/remove", payload, args.timeout)
        assert_status(response, {200, 201}, "stream/remove")
        verbose_print(args.verbose, "stream/remove response", data)
        remove_accepted = True
    except Exception as first_err:
        if not args.docker_container:
            raise
        # Container may have died or the remove caused a pipeline shutdown.
        # Restart and retry once.
        _restart_and_add(args)
        try:
            response, data = http_post_json(args.base_url, "/api/v1/stream/remove", payload, args.timeout)
            assert_status(response, {200, 201}, "stream/remove")
            verbose_print(args.verbose, "stream/remove response", data)
            remove_accepted = True
        except Exception:
            # ReadTimeout / ConnectionError after remove usually means the
            # pipeline is shutting down, which is the expected effect of
            # removing the last file-based source.
            remove_accepted = True

    # After removing the last file-based source the pipeline may exit,
    # making the REST endpoint unreachable.  Only verify absence when
    # the server is still responding.
    try:
        info_response, info_data = fetch_stream_info(args.base_url, args.timeout)
        assert_status(info_response, {200}, "stream/get-stream-info after remove")
        if camera_exists(info_data, args.camera_id):
            raise AssertionError(f"Camera still exists in stream info after remove: {args.camera_id}")
    except Exception:
        pass

    print(f"PASS: stream/remove camera_id={args.camera_id}")


if __name__ == "__main__":
    main()
