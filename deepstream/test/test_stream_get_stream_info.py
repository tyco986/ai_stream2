#!/usr/bin/env python3
"""
Test GET /api/v1/stream/get-stream-info

Example:
  python3 deepstream/test/test_stream_get_stream_info.py --base-url http://127.0.0.1:9000
"""

from _common import assert_status, create_parser, fetch_stream_info, verbose_print


def main():
    parser = create_parser("Test DeepStream stream/get-stream-info API")
    args = parser.parse_args()

    response, data = fetch_stream_info(args.base_url, args.timeout)
    assert_status(response, {200}, "stream/get-stream-info")

    if not isinstance(data, (dict, list)):
        raise AssertionError(f"Stream info should be JSON object/list, got: {type(data)}")

    verbose_print(args.verbose, "stream/get-stream-info response", data)
    print("PASS: stream/get-stream-info")


if __name__ == "__main__":
    main()
