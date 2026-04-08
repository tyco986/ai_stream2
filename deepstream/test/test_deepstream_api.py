"""DeepStream REST + Kafka command integration tests.

Tests are defined in execution order within the class — pytest runs
methods in definition order, which matters because:
  1. health / stream-info are read-only checks
  2. stream/add verifies the camera added by the session fixture
  3. command tests operate on the active camera
  4. stream/remove is intentionally last (it tears down the camera)
"""

import time

from _common import (
    assert_status,
    build_stream_remove_payload,
    camera_exists,
    fetch_stream_info,
    find_source_id_by_camera_id,
    get_screenshots_dir,
    http_get_json,
    http_post_json,
    parse_source_id,
    send_command,
    wait_for_file,
    write_test_payload,
)


class TestDeepStreamAPI:
    """Ordered integration tests against a running DeepStream instance."""

    # ---- health ----

    def test_health_ready_state(self, ds):
        response, data = http_get_json(ds.base_url, "/api/v1/health/get-dsready-state", ds.timeout)
        assert_status(response, {200}, "health/get-dsready-state")
        assert isinstance(data, dict), f"Health response should be dict, got {type(data)}"

        ready_keys = {"ready", "is_ready", "dsready", "dsReady"}
        top_level_ok = bool(ready_keys & data.keys())
        health_info = data.get("health-info")
        nested_ok = isinstance(health_info, dict) and bool(
            {"ds-ready", "ds_ready", "ready"} & health_info.keys()
        )
        assert top_level_ok or nested_ok, f"Health response missing ready field, keys={list(data.keys())}"

    # ---- stream info ----

    def test_stream_info(self, ds, prepared_camera):
        response, data = fetch_stream_info(ds.base_url, ds.timeout)
        assert_status(response, {200}, "stream/get-stream-info")
        assert isinstance(data, (dict, list)), f"Stream info should be dict/list, got {type(data)}"

    # ---- stream add ----

    def test_stream_add(self, ds, prepared_camera):
        response, data = fetch_stream_info(ds.base_url, ds.timeout)
        assert_status(response, {200}, "stream/get-stream-info after add")
        assert camera_exists(data, ds.camera_id), f"Camera not found after add: {ds.camera_id}"
        source_id = find_source_id_by_camera_id(data, ds.camera_id)
        assert source_id is not None, f"source_id not found for {ds.camera_id}"

    # ---- Kafka commands ----

    def test_command_start_rolling(self, ds, prepared_camera, kafka_producer):
        payload = {"action": "start_rolling", "source_id": ds.camera_id}
        write_test_payload(f"{ds.camera_id}_command_start_rolling.json", payload)
        send_command(kafka_producer, ds.command_topic, payload, ds.timeout)

    def test_command_stop_rolling(self, ds, prepared_camera, kafka_producer):
        start_payload = {"action": "start_rolling", "source_id": ds.camera_id}
        send_command(kafka_producer, ds.command_topic, start_payload, ds.timeout)

        payload = {"action": "stop_rolling", "source_id": ds.camera_id}
        write_test_payload(f"{ds.camera_id}_command_stop_rolling.json", payload)
        send_command(kafka_producer, ds.command_topic, payload, ds.timeout)

    def test_command_start_recording_event(self, ds, prepared_camera, kafka_producer):
        payload = {
            "action": "start_recording",
            "source_id": ds.camera_id,
            "duration": 20,
            "type": "event",
        }
        write_test_payload(f"{ds.camera_id}_command_start_recording_event.json", payload)
        send_command(kafka_producer, ds.command_topic, payload, ds.timeout)

    def test_command_start_recording_manual(self, ds, prepared_camera, kafka_producer):
        payload = {
            "action": "start_recording",
            "source_id": ds.camera_id,
            "duration": 0,
            "type": "manual",
        }
        write_test_payload(f"{ds.camera_id}_command_start_recording_manual.json", payload)
        send_command(kafka_producer, ds.command_topic, payload, ds.timeout)

    def test_command_stop_recording(self, ds, prepared_camera, kafka_producer):
        start_payload = {
            "action": "start_recording",
            "source_id": ds.camera_id,
            "duration": 0,
            "type": "manual",
        }
        send_command(kafka_producer, ds.command_topic, start_payload, ds.timeout)

        payload = {"action": "stop_recording", "source_id": ds.camera_id}
        write_test_payload(f"{ds.camera_id}_command_stop_recording.json", payload)
        send_command(kafka_producer, ds.command_topic, payload, ds.timeout)

    def test_command_screenshot(self, ds, prepared_camera, kafka_producer):
        screenshot_dir = get_screenshots_dir()
        screenshot_dir.mkdir(parents=True, exist_ok=True)
        filename = f"{ds.camera_id}_{int(time.time())}.jpg"
        expected_file = screenshot_dir / filename

        payload = {"action": "screenshot", "source_id": ds.camera_id, "filename": filename}
        write_test_payload(f"{ds.camera_id}_command_screenshot.json", payload)
        send_command(kafka_producer, ds.command_topic, payload, ds.timeout)

        assert wait_for_file(expected_file, wait_seconds=20), f"Screenshot file not found: {expected_file}"

    def test_command_switch_preview(self, ds, prepared_camera, kafka_producer):
        _, info_data = fetch_stream_info(ds.base_url, ds.timeout)
        source_id = parse_source_id(info_data, ds.camera_id)

        payload_single = {"action": "switch_preview", "source_id": source_id}
        payload_multi = {"action": "switch_preview", "source_id": -1}
        write_test_payload(f"{ds.camera_id}_command_switch_preview_single.json", payload_single)
        write_test_payload(f"{ds.camera_id}_command_switch_preview_multi.json", payload_multi)

        send_command(kafka_producer, ds.command_topic, payload_single, ds.timeout)
        send_command(kafka_producer, ds.command_topic, payload_multi, ds.timeout)

    def test_command_toggle_osd(self, ds, prepared_camera, kafka_producer):
        payload_off = {"action": "toggle_osd", "show": False}
        payload_on = {"action": "toggle_osd", "show": True}
        write_test_payload(f"{ds.camera_id}_command_toggle_osd_off.json", payload_off)
        write_test_payload(f"{ds.camera_id}_command_toggle_osd_on.json", payload_on)

        send_command(kafka_producer, ds.command_topic, payload_off, ds.timeout)
        send_command(kafka_producer, ds.command_topic, payload_on, ds.timeout)

    # ---- stream remove (must be last) ----

    def test_stream_remove(self, ds, prepared_camera, kafka_producer):
        payload = build_stream_remove_payload(ds.camera_id, ds.camera_url)
        write_test_payload(f"{ds.camera_id}_stream_remove.json", payload)

        response, data = http_post_json(ds.base_url, "/api/v1/stream/remove", payload, ds.timeout)
        assert_status(response, {200, 201}, "stream/remove")

        response, info_data = fetch_stream_info(ds.base_url, ds.timeout)
        assert_status(response, {200}, "stream/get-stream-info after remove")
        assert not camera_exists(info_data, ds.camera_id), \
            f"Camera still exists after remove: {ds.camera_id}"
