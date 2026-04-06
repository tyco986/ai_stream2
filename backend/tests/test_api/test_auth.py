"""Auth & permissions API tests."""
import pytest
from rest_framework_simplejwt.tokens import RefreshToken

from tests.factories import (
    AIModelFactory,
    AlertFactory,
    AlertRuleFactory,
    CameraFactory,
    DetectionFactory,
    PipelineProfileFactory,
    UserFactory,
)


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

    def test_inactive_user_cannot_login(self, api_client, org):
        user = UserFactory(organization=org, is_active=False)
        resp = api_client.post("/api/v1/auth/login/", {
            "username": user.username,
            "password": "testpass123",
        })
        assert resp.status_code == 401


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

    def test_viewer_cannot_create_alert_rule(self, viewer_client, org):
        resp = viewer_client.post("/api/v1/alert-rules/", {
            "name": "Test Rule",
            "rule_type": "object_count",
            "conditions": {"min_count": 5},
        }, format="json")
        assert resp.status_code == 403

    def test_viewer_cannot_acknowledge_alert(self, viewer_client, org):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(rule=rule, camera=camera, organization=org)
        resp = viewer_client.post(f"/api/v1/alerts/{alert.id}/acknowledge/")
        assert resp.status_code == 403

    def test_viewer_cannot_resolve_alert(self, viewer_client, org):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(
            rule=rule, camera=camera, organization=org, status="acknowledged",
        )
        resp = viewer_client.post(f"/api/v1/alerts/{alert.id}/resolve/")
        assert resp.status_code == 403

    def test_viewer_cannot_create_ai_model(self, viewer_client, org):
        resp = viewer_client.post("/api/v1/ai-models/", {
            "name": "yolov8",
            "model_type": "detector",
            "model_file": "/models/yolov8.onnx",
            "config": {"num_classes": 80},
        }, format="json")
        assert resp.status_code == 403

    def test_viewer_cannot_create_pipeline(self, viewer_client, org):
        detector = AIModelFactory(organization=org, model_type="detector")
        resp = viewer_client.post("/api/v1/pipeline-profiles/", {
            "name": "Pipeline",
            "detector": str(detector.id),
        }, format="json")
        assert resp.status_code == 403

    def test_viewer_can_list_alert_rules(self, viewer_client, org):
        AlertRuleFactory(organization=org)
        resp = viewer_client.get("/api/v1/alert-rules/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1

    def test_viewer_can_list_ai_models(self, viewer_client, org):
        AIModelFactory(organization=org)
        resp = viewer_client.get("/api/v1/ai-models/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1

    def test_viewer_can_list_pipelines(self, viewer_client, org):
        PipelineProfileFactory(organization=org)
        resp = viewer_client.get("/api/v1/pipeline-profiles/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1


@pytest.mark.django_db
class TestAdminPermissions:
    """Admin role has the same write permissions as operator via IsOperatorOrAbove."""

    def test_admin_can_create_camera(self, admin_client, org):
        resp = admin_client.post("/api/v1/cameras/", {
            "uid": "cam-admin",
            "name": "Admin Camera",
            "rtsp_url": "rtsp://10.0.0.1/stream",
        })
        assert resp.status_code == 201

    def test_admin_can_delete_camera(self, admin_client, camera):
        resp = admin_client.delete(f"/api/v1/cameras/{camera.id}/")
        assert resp.status_code == 204

    def test_admin_can_create_alert_rule(self, admin_client, org):
        resp = admin_client.post("/api/v1/alert-rules/", {
            "name": "Admin Rule",
            "rule_type": "object_count",
            "conditions": {"min_count": 3},
            "cooldown_seconds": 60,
        }, format="json")
        assert resp.status_code == 201

    def test_admin_can_acknowledge_alert(self, admin_client, org, admin_user):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(rule=rule, camera=camera, organization=org)
        resp = admin_client.post(f"/api/v1/alerts/{alert.id}/acknowledge/")
        assert resp.status_code == 200
        assert resp.data["data"]["acknowledged_by"] == admin_user.id

    def test_admin_can_create_ai_model(self, admin_client, org):
        resp = admin_client.post("/api/v1/ai-models/", {
            "name": "admin-model",
            "model_type": "detector",
            "model_file": "/models/model.onnx",
            "config": {"num_classes": 80},
        }, format="json")
        assert resp.status_code == 201

    def test_admin_can_create_pipeline(self, admin_client, org):
        detector = AIModelFactory(organization=org, model_type="detector")
        resp = admin_client.post("/api/v1/pipeline-profiles/", {
            "name": "Admin Pipeline",
            "detector": str(detector.id),
        }, format="json")
        assert resp.status_code == 201


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
        camera_b = CameraFactory(organization=org_b)
        rule_b = AlertRuleFactory(organization=org_b)
        alert_b = AlertFactory(rule=rule_b, camera=camera_b, organization=org_b)
        resp = operator_client.post(f"/api/v1/alerts/{alert_b.id}/acknowledge/")
        assert resp.status_code == 404

    def test_org_a_cannot_see_org_b_detections(self, viewer_client, org, org_b):
        cam_a = CameraFactory(organization=org, uid="cam-iso-a")
        cam_b = CameraFactory(organization=org_b, uid="cam-iso-b")
        DetectionFactory(camera=cam_a)
        DetectionFactory(camera=cam_b)
        DetectionFactory(camera=cam_b)

        resp = viewer_client.get("/api/v1/detections/")
        assert resp.status_code == 200
        assert len(resp.data["results"]) == 1

    def test_org_a_cannot_see_org_b_alert_rules(self, viewer_client, org, org_b):
        AlertRuleFactory(organization=org)
        AlertRuleFactory(organization=org_b)
        resp = viewer_client.get("/api/v1/alert-rules/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1

    def test_org_a_cannot_see_org_b_alerts_list(self, viewer_client, org, org_b):
        cam_a = CameraFactory(organization=org)
        cam_b = CameraFactory(organization=org_b)
        rule_a = AlertRuleFactory(organization=org)
        rule_b = AlertRuleFactory(organization=org_b)
        AlertFactory(rule=rule_a, camera=cam_a, organization=org)
        AlertFactory(rule=rule_b, camera=cam_b, organization=org_b)
        AlertFactory(rule=rule_b, camera=cam_b, organization=org_b)

        resp = viewer_client.get("/api/v1/alerts/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1

    def test_org_a_cannot_see_org_b_ai_models(self, viewer_client, org, org_b):
        AIModelFactory(organization=org)
        AIModelFactory(organization=org_b)
        resp = viewer_client.get("/api/v1/ai-models/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1

    def test_org_a_cannot_see_org_b_pipelines(self, viewer_client, org, org_b):
        PipelineProfileFactory(organization=org)
        PipelineProfileFactory(organization=org_b)
        resp = viewer_client.get("/api/v1/pipeline-profiles/")
        assert resp.status_code == 200
        assert resp.data["count"] == 1

    def test_org_a_cannot_resolve_org_b_alert(self, operator_client, org_b):
        camera_b = CameraFactory(organization=org_b)
        rule_b = AlertRuleFactory(organization=org_b)
        alert_b = AlertFactory(
            rule=rule_b, camera=camera_b, organization=org_b,
            status="acknowledged",
        )
        resp = operator_client.post(f"/api/v1/alerts/{alert_b.id}/resolve/")
        assert resp.status_code == 404

    def test_org_b_client_sees_only_own_data(self, org_b_client, org, org_b):
        CameraFactory(organization=org, uid="cam-a-only")
        CameraFactory(organization=org_b, uid="cam-b-only")
        resp = org_b_client.get("/api/v1/cameras/")
        assert resp.status_code == 200
        uids = [c["uid"] for c in resp.data["results"]]
        assert "cam-b-only" in uids
        assert "cam-a-only" not in uids

    def test_dashboard_overview_isolated_by_org(
        self, operator_client, org_b_client, org, org_b,
    ):
        CameraFactory(organization=org, status="online")
        CameraFactory(organization=org, status="offline")
        CameraFactory(organization=org_b, status="online")
        CameraFactory(organization=org_b, status="online")
        CameraFactory(organization=org_b, status="online")

        resp_a = operator_client.get("/api/v1/dashboard/overview/")
        assert resp_a.status_code == 200
        assert resp_a.data["data"]["total_cameras"] == 2
        assert resp_a.data["data"]["online_cameras"] == 1

        resp_b = org_b_client.get("/api/v1/dashboard/overview/")
        assert resp_b.status_code == 200
        assert resp_b.data["data"]["total_cameras"] == 3
        assert resp_b.data["data"]["online_cameras"] == 3
