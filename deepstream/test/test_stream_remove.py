#!/usr/bin/env python3
"""
Test POST /api/v1/stream/remove

Example:
  python3 deepstream/test/test_stream_remove.py --camera-url rtsp://127.0.0.1:8554/video1
"""

from _common import (
    assert_status,
    build_stream_remove_payload,
    camera_exists,
    create_parser,
    fetch_stream_info,
    http_post_json,
    prepare_camera,
    verbose_print,
    write_test_payload,
)


def main():
    parser = create_parser("Test DeepStream stream/remove API")
    args = parser.parse_args()

    prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)

    payload = build_stream_remove_payload(args.camera_id, args.camera_url)
    write_test_payload(f"{args.camera_id}_stream_remove.json", payload)

    response, data = http_post_json(args.base_url, "/api/v1/stream/remove", payload, args.timeout)
    assert_status(response, {200, 201}, "stream/remove")
    verbose_print(args.verbose, "stream/remove response", data)

    info_response, info_data = fetch_stream_info(args.base_url, args.timeout)
    assert_status(info_response, {200}, "stream/get-stream-info after remove")
    if camera_exists(info_data, args.camera_id):
        raise AssertionError(f"Camera still exists in stream info after remove: {args.camera_id}")

    print(f"PASS: stream/remove camera_id={args.camera_id}")


if __name__ == "__main__":
    main()
