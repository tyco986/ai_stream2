"""Dashboard API tests."""
import pytest

from tests.factories import CameraFactory, DetectionFactory


@pytest.mark.django_db
class TestDashboard:
    def test_overview(self, operator_client, org):
        CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="offline")
        resp = operator_client.get("/api/v1/dashboard/overview/")
        assert resp.status_code == 200
        data = resp.data["data"]
        assert data["total_cameras"] == 2
        assert data["online_cameras"] == 1

    def test_detection_trend(self, operator_client, org):
        cam = CameraFactory(organization=org)
        DetectionFactory(camera=cam)
        resp = operator_client.get("/api/v1/dashboard/detection-trend/")
        assert resp.status_code == 200
        assert "data" in resp.data

    def test_camera_status(self, operator_client, org):
        CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="offline")
        resp = operator_client.get("/api/v1/dashboard/camera-status/")
        assert resp.status_code == 200
        assert "data" in resp.data
