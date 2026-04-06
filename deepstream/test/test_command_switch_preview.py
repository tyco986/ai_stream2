#!/usr/bin/env python3
"""
Test command action: switch_preview

Example:
  python3 deepstream/test/test_command_switch_preview.py \
    --base-url http://127.0.0.1:9000 --kafka-broker 127.0.0.1:9092
"""

from _common import (
    build_producer,
    create_parser,
    fetch_stream_info,
    parse_source_id,
    prepare_camera,
    send_command,
    write_test_payload,
)


def main():
    parser = create_parser("Test DeepStream command switch_preview", include_kafka=True)
    parser.add_argument("--no-prepare", action="store_true", help="Use pre-existing camera source")
    args = parser.parse_args()

    if args.no_prepare:
        _, info_data = fetch_stream_info(args.base_url, args.timeout)
        source_id = parse_source_id(info_data, args.camera_id)
    else:
        source_id = prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)

    producer = build_producer(args.kafka_broker)
    payload_single = {"action": "switch_preview", "source_id": source_id}
    payload_multi = {"action": "switch_preview", "source_id": -1}
    write_test_payload(f"{args.camera_id}_command_switch_preview_single.json", payload_single)
    write_test_payload(f"{args.camera_id}_command_switch_preview_multi.json", payload_multi)

    send_command(producer, args.command_topic, payload_single, args.timeout)
    send_command(producer, args.command_topic, payload_multi, args.timeout)

    print(f"PASS: command switch_preview source_id={source_id} then -1")


if __name__ == "__main__":
    main()
