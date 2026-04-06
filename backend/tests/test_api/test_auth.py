"""Auth & permissions API tests."""
import pytest
from rest_framework_simplejwt.tokens import RefreshToken

from tests.factories import UserFactory


@pytest.mark.django_db
class TestAuthEndpoints:
    def test_login_returns_tokens(self, api_client, operator_user):
        resp = api_client.post("/api/v1/auth/login/", {
            "username": operator_user.username,
            "password": "testpass123",
        })
        assert resp.status_code == 200
        assert "access" in resp.data
        assert "refresh" in resp.data

    def test_login_wrong_password(self, api_client, operator_user):
        resp = api_client.post("/api/v1/auth/login/", {
            "username": operator_user.username,
            "password": "wrong",
        })
        assert resp.status_code == 401

    def test_me_without_token(self, api_client):
        resp = api_client.get("/api/v1/auth/me/")
        assert resp.status_code == 401

    def test_me_with_valid_token(self, operator_client, operator_user):
        resp = operator_client.get("/api/v1/auth/me/")
        assert resp.status_code == 200
        assert resp.data["data"]["username"] == operator_user.username

    def test_refresh_returns_new_access_token(self, api_client, operator_user):
        refresh = RefreshToken.for_user(operator_user)
        resp = api_client.post("/api/v1/auth/refresh/", {
            "refresh": str(refresh),
        })
        assert resp.status_code == 200
        assert "access" in resp.data


@pytest.mark.django_db
class TestPermissions:
    def test_viewer_cannot_create_camera(self, viewer_client, org):
        resp = viewer_client.post("/api/v1/cameras/", {
            "uid": "cam-new",
            "name": "New",
            "rtsp_url": "rtsp://x/stream",
        })
        assert resp.status_code == 403

    def test_operator_can_create_camera(self, operator_client, org):
        resp = operator_client.post("/api/v1/cameras/", {
            "uid": "cam-new",
            "name": "New Camera",
            "rtsp_url": "rtsp://x/stream",
        })
        assert resp.status_code == 201

    def test_viewer_can_list_cameras(self, viewer_client, camera):
        resp = viewer_client.get("/api/v1/cameras/")
        assert resp.status_code == 200


@pytest.mark.django_db
class TestMultiTenantIsolation:
    def test_org_a_cannot_see_org_b_cameras(self, operator_client, camera_b):
        resp = operator_client.get("/api/v1/cameras/")
        assert resp.status_code == 200
        camera_ids = [c["id"] for c in resp.data["results"]]
        assert str(camera_b.id) not in camera_ids

    def test_org_a_cannot_access_org_b_camera_detail(self, operator_client, camera_b):
        resp = operator_client.get(f"/api/v1/cameras/{camera_b.id}/")
        assert resp.status_code == 404

    def test_org_a_cannot_acknowledge_org_b_alert(self, operator_client, org_b):
        from tests.factories import AlertFactory, AlertRuleFactory, CameraFactory
        camera_b = CameraFactory(organization=org_b)
        rule_b = AlertRuleFactory(organization=org_b)
        alert_b = AlertFactory(rule=rule_b, camera=camera_b, organization=org_b)
        resp = operator_client.post(f"/api/v1/alerts/{alert_b.id}/acknowledge/")
        assert resp.status_code == 404
