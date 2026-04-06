#!/usr/bin/env python3
"""
Test GET /api/v1/health/get-dsready-state

Example:
  python3 deepstream/test/test_health_get_dsready_state.py --base-url http://127.0.0.1:9000
"""

from _common import assert_status, create_parser, http_get_json, verbose_print


def main():
    parser = create_parser("Test DeepStream health API")
    args = parser.parse_args()

    response, data = http_get_json(args.base_url, "/api/v1/health/get-dsready-state", args.timeout)
    assert_status(response, {200}, "health/get-dsready-state")
    if not isinstance(data, dict):
        raise AssertionError(f"Health response should be JSON object, got: {type(data)}")

    ready_keys = ["ready", "is_ready", "dsready", "dsReady"]
    top_level_ok = any(key in data for key in ready_keys)
    nested_ok = (
        isinstance(data.get("health-info"), dict)
        and (
            "ds-ready" in data["health-info"]
            or "ds_ready" in data["health-info"]
            or "ready" in data["health-info"]
        )
    )
    if not top_level_ok and not nested_ok:
        raise AssertionError(f"Health response missing ready field, keys={list(data.keys())}")

    verbose_print(args.verbose, "health response", data)
    print("PASS: health/get-dsready-state")


if __name__ == "__main__":
    main()
