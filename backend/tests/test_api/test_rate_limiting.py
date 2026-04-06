"""Rate limiting tests — login throttle."""
from unittest.mock import patch

import pytest
from django.core.cache import cache

from tests.factories import UserFactory


@pytest.mark.django_db
class TestLoginRateLimit:
    @pytest.fixture(autouse=True)
    def _clear_throttle_cache(self):
        cache.clear()
        yield
        cache.clear()

    def test_login_throttled_after_limit(self, api_client, org):
        user = UserFactory(organization=org)
        payload = {"username": user.username, "password": "testpass123"}

        with patch(
            "common.throttles.LoginRateThrottle.THROTTLE_RATES",
            {"login": "3/minute"},
        ):
            for _ in range(3):
                resp = api_client.post("/api/v1/auth/login/", payload)
                assert resp.status_code == 200

            resp = api_client.post("/api/v1/auth/login/", payload)
            assert resp.status_code == 429

    def test_throttle_returns_proper_error_code(self, api_client, org):
        user = UserFactory(organization=org)
        payload = {"username": user.username, "password": "testpass123"}

        with patch(
            "common.throttles.LoginRateThrottle.THROTTLE_RATES",
            {"login": "2/minute"},
        ):
            for _ in range(2):
                api_client.post("/api/v1/auth/login/", payload)

            resp = api_client.post("/api/v1/auth/login/", payload)
            assert resp.status_code == 429
            assert resp.data["code"] == "THROTTLED"
