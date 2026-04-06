#!/usr/bin/env python3
"""
Test command action: start_rolling

Example:
  python3 deepstream/test/test_command_start_rolling.py \
    --base-url http://127.0.0.1:9000 --kafka-broker 127.0.0.1:9092
"""

from _common import (
    build_producer,
    create_parser,
    prepare_camera,
    send_command,
    write_test_payload,
)


def main():
    parser = create_parser("Test DeepStream command start_rolling", include_kafka=True)
    parser.add_argument("--no-prepare", action="store_true", help="Use pre-existing camera source")
    args = parser.parse_args()

    if not args.no_prepare:
        prepare_camera(args.base_url, args.timeout, args.camera_id, args.camera_name, args.camera_url)

    payload = {"action": "start_rolling", "source_id": args.camera_id}
    write_test_payload(f"{args.camera_id}_command_start_rolling.json", payload)

    producer = build_producer(args.kafka_broker)
    send_command(producer, args.command_topic, payload, args.timeout)

    print(f"PASS: command start_rolling sensor_id={args.camera_id}")


if __name__ == "__main__":
    main()
