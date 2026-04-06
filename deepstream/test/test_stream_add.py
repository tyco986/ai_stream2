#!/usr/bin/env python3
"""
Test POST /api/v1/stream/add

Example:
  python3 deepstream/test/test_stream_add.py --camera-url rtsp://127.0.0.1:8554/video1
"""

from _common import (
    assert_status,
    camera_exists,
    create_parser,
    fetch_stream_info,
    find_source_id_by_camera_id,
    prepare_camera,
    verbose_print,
)


def main():
    parser = create_parser("Test DeepStream stream/add API")
    parser.add_argument("--no-prepare", action="store_true", help="Skip reset; assume camera already added")
    args = parser.parse_args()

    if args.no_prepare:
        _, info_data = fetch_stream_info(args.base_url, args.timeout)
        source_id = find_source_id_by_camera_id(info_data, args.camera_id)
        if source_id is None:
            raise AssertionError(f"--no-prepare but camera {args.camera_id} not found in stream info")
    else:
        source_id = prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)

    info_response, info_data = fetch_stream_info(args.base_url, args.timeout)
    assert_status(info_response, {200}, "stream/get-stream-info after add")
    if not camera_exists(info_data, args.camera_id):
        raise AssertionError(f"Camera was not found in stream info after add: {args.camera_id}")
    verbose_print(args.verbose, "stream/get-stream-info response", info_data)

    print(f"PASS: stream/add camera_id={args.camera_id} source_id={source_id}")


if __name__ == "__main__":
    main()
