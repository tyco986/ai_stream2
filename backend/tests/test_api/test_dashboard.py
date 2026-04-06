"""Dashboard API tests."""
from datetime import timedelta

import pytest
from django.utils.timezone import now

from tests.factories import (
    AlertFactory,
    AlertRuleFactory,
    CameraFactory,
    DetectionFactory,
)


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

    def test_unauthenticated_denied(self, api_client):
        resp = api_client.get("/api/v1/dashboard/overview/")
        assert resp.status_code == 401


@pytest.mark.django_db
class TestDashboardCorrectness:
    def test_overview_counts_all_fields(self, operator_client, org):
        cam1 = CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="offline")
        CameraFactory(organization=org, status="error")
        DetectionFactory(camera=cam1)
        DetectionFactory(camera=cam1)
        rule = AlertRuleFactory(organization=org)
        AlertFactory(rule=rule, camera=cam1, organization=org, status="pending")
        AlertFactory(rule=rule, camera=cam1, organization=org, status="acknowledged")

        resp = operator_client.get("/api/v1/dashboard/overview/")
        assert resp.status_code == 200
        data = resp.data["data"]
        assert data["total_cameras"] == 3
        assert data["online_cameras"] == 1
        assert data["today_detections"] == 2
        assert data["pending_alerts"] == 1

    def test_overview_excludes_deleted_cameras(self, operator_client, org):
        CameraFactory(organization=org, status="online")
        deleted = CameraFactory(organization=org, status="online")
        deleted.is_deleted = True
        deleted.save()

        resp = operator_client.get("/api/v1/dashboard/overview/")
        data = resp.data["data"]
        assert data["total_cameras"] == 1
        assert data["online_cameras"] == 1

    def test_camera_status_distribution(self, operator_client, org):
        CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="offline")
        CameraFactory(organization=org, status="error")

        resp = operator_client.get("/api/v1/dashboard/camera-status/")
        assert resp.status_code == 200
        status_map = {s["status"]: s["count"] for s in resp.data["data"]}
        assert status_map["online"] == 2
        assert status_map["offline"] == 1
        assert status_map["error"] == 1

    def test_detection_trend_with_hours_param(self, operator_client, org):
        cam = CameraFactory(organization=org)
        recent = now() - timedelta(hours=1)
        old = now() - timedelta(hours=48)
        DetectionFactory(camera=cam, detected_at=recent)
        DetectionFactory(camera=cam, detected_at=old)

        resp = operator_client.get("/api/v1/dashboard/detection-trend/?hours=6")
        assert resp.status_code == 200
        total = sum(item["count"] for item in resp.data["data"])
        assert total == 1

    def test_empty_dashboard(self, operator_client, org):
        resp = operator_client.get("/api/v1/dashboard/overview/")
        assert resp.status_code == 200
        data = resp.data["data"]
        assert data["total_cameras"] == 0
        assert data["online_cameras"] == 0
        assert data["today_detections"] == 0
        assert data["pending_alerts"] == 0
