"""Health check API tests."""
from unittest.mock import patch

import pytest


@pytest.mark.django_db
class TestHealthEndpoints:
    def test_liveness(self, api_client):
        resp = api_client.get("/api/v1/health/live/")
        assert resp.status_code == 200
        assert resp.data["status"] == "alive"

    def test_readiness_healthy(self, api_client):
        resp = api_client.get("/api/v1/health/ready/")
        assert resp.status_code == 200
        assert resp.data["status"] == "healthy"
        assert resp.data["checks"]["database"] is True

    @patch("apps.dashboard.health_views.cache")
    def test_readiness_redis_down(self, mock_cache, api_client):
        mock_cache.set.side_effect = Exception("Connection refused")
        resp = api_client.get("/api/v1/health/ready/")
        assert resp.status_code == 503
        assert resp.data["status"] == "degraded"
        assert resp.data["checks"]["redis"] is False
