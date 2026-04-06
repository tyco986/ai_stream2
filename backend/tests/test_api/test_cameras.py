"""Camera CRUD + stream lifecycle API tests."""
from unittest.mock import AsyncMock, patch

import pytest

from tests.factories import CameraFactory


@pytest.mark.django_db
class TestCameraCRUD:
    def test_create_list_update_soft_delete(self, operator_client, org):
        # Create
        resp = operator_client.post("/api/v1/cameras/", {
            "uid": "cam-lifecycle",
            "name": "Lifecycle Camera",
            "rtsp_url": "rtsp://10.0.0.1/stream",
        })
        assert resp.status_code == 201

        # List — find by uid since CameraWriteSerializer may not return id
        resp = operator_client.get("/api/v1/cameras/")
        assert resp.status_code == 200
        results = resp.data.get("results", [])
        matches = [c for c in results if c["uid"] == "cam-lifecycle"]
        assert len(matches) == 1
        camera_id = str(matches[0]["id"])

        # Update name
        resp = operator_client.patch(f"/api/v1/cameras/{camera_id}/", {
            "name": "Renamed Camera",
        })
        assert resp.status_code == 200

        # Soft delete
        resp = operator_client.delete(f"/api/v1/cameras/{camera_id}/")
        assert resp.status_code == 204

        # No longer visible in list
        resp = operator_client.get("/api/v1/cameras/")
        results = resp.data.get("results", [])
        assert not any(c["uid"] == "cam-lifecycle" for c in results)


@pytest.mark.django_db
class TestStartStream:
    @patch("apps.cameras.views.deepstream_client")
    def test_start_stream_success(self, mock_ds, operator_client, camera):
        mock_ds.add_stream = AsyncMock()
        resp = operator_client.post(f"/api/v1/cameras/{camera.id}/start-stream/")
        assert resp.status_code == 200
        assert resp.data["data"]["status"] == "connecting"

    @patch("apps.cameras.views.deepstream_client")
    def test_start_stream_idempotent(self, mock_ds, operator_client, camera):
        camera.status = "online"
        camera.save()
        resp = operator_client.post(f"/api/v1/cameras/{camera.id}/start-stream/")
        assert resp.status_code == 200
        assert resp.data["data"]["status"] == "online"
        mock_ds.add_stream.assert_not_called()

    @patch("apps.cameras.views.deepstream_client")
    def test_start_stream_deepstream_unavailable(self, mock_ds, operator_client, camera):
        mock_ds.add_stream = AsyncMock(side_effect=Exception("connection refused"))
        resp = operator_client.post(f"/api/v1/cameras/{camera.id}/start-stream/")
        assert resp.status_code == 503
        assert resp.data["code"] == "DEEPSTREAM_UNAVAILABLE"

        camera.refresh_from_db()
        assert camera.status == "offline"


@pytest.mark.django_db
class TestStopStream:
    @patch("apps.cameras.views.deepstream_client")
    def test_stop_stream_success(self, mock_ds, operator_client, camera):
        camera.status = "online"
        camera.save()
        mock_ds.remove_stream = AsyncMock()
        resp = operator_client.post(f"/api/v1/cameras/{camera.id}/stop-stream/")
        assert resp.status_code == 200
        assert resp.data["data"]["status"] == "offline"

    @patch("apps.cameras.views.deepstream_client")
    def test_stop_stream_already_offline(self, mock_ds, operator_client, camera):
        resp = operator_client.post(f"/api/v1/cameras/{camera.id}/stop-stream/")
        assert resp.status_code == 200
        mock_ds.remove_stream.assert_not_called()
