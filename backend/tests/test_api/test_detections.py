"""Detection API tests — list, filtering, cursor pagination, stats."""
from datetime import timedelta

import pytest
from django.utils.timezone import now

from tests.factories import CameraFactory, DetectionFactory


@pytest.mark.django_db
class TestDetectionList:
    def test_list_detections(self, viewer_client, camera):
        DetectionFactory(camera=camera)
        DetectionFactory(camera=camera)
        resp = viewer_client.get("/api/v1/detections/")
        assert resp.status_code == 200
        assert len(resp.data["results"]) == 2

    def test_filter_by_camera_id(self, viewer_client, org):
        cam1 = CameraFactory(organization=org, uid="cam-filter-1")
        cam2 = CameraFactory(organization=org, uid="cam-filter-2")
        DetectionFactory(camera=cam1)
        DetectionFactory(camera=cam2)
        DetectionFactory(camera=cam2)

        resp = viewer_client.get(f"/api/v1/detections/?camera_id={cam1.id}")
        assert resp.status_code == 200
        assert len(resp.data["results"]) == 1

    def test_filter_by_time_range(self, viewer_client, camera):
        base = now()
        old = base - timedelta(hours=48)
        recent = base - timedelta(minutes=30)
        d_old = DetectionFactory(camera=camera, detected_at=old)
        d_recent = DetectionFactory(camera=camera, detected_at=recent)

        start = (base - timedelta(hours=2)).strftime("%Y-%m-%dT%H:%M:%SZ")
        end = (base + timedelta(minutes=1)).strftime("%Y-%m-%dT%H:%M:%SZ")
        resp = viewer_client.get(f"/api/v1/detections/?start_time={start}&end_time={end}")
        assert resp.status_code == 200
        result_ids = [r["id"] for r in resp.data["results"]]
        assert d_recent.id in result_ids
        assert d_old.id not in result_ids

    def test_cursor_pagination(self, viewer_client, camera):
        for _ in range(5):
            DetectionFactory(camera=camera)
        resp = viewer_client.get("/api/v1/detections/?page_size=2")
        assert resp.status_code == 200
        assert len(resp.data["results"]) == 2
        assert resp.data["next"] is not None

    def test_stats_endpoint(self, viewer_client, camera):
        DetectionFactory(camera=camera)
        resp = viewer_client.get("/api/v1/detections/stats/")
        assert resp.status_code == 200
        assert "data" in resp.data
