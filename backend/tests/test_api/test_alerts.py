"""Alert workflow API tests — create rules, trigger alerts, state transitions."""
import pytest

from tests.factories import AlertFactory, AlertRuleFactory, CameraFactory


@pytest.mark.django_db
class TestAlertRuleCRUD:
    def test_create_alert_rule(self, operator_client, org, camera):
        resp = operator_client.post("/api/v1/alert-rules/", {
            "name": "High traffic",
            "rule_type": "object_count",
            "conditions": {"min_count": 10},
            "cooldown_seconds": 120,
            "notify_channels": ["websocket"],
        }, format="json")
        assert resp.status_code == 201
        assert resp.data["rule_type"] == "object_count"

    def test_list_alert_rules(self, viewer_client, org):
        AlertRuleFactory(organization=org)
        AlertRuleFactory(organization=org)
        resp = viewer_client.get("/api/v1/alert-rules/")
        assert resp.status_code == 200
        assert resp.data["count"] == 2


@pytest.mark.django_db
class TestAlertWorkflow:
    def test_acknowledge_pending_alert(self, operator_client, org):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(rule=rule, camera=camera, organization=org, status="pending")

        resp = operator_client.post(f"/api/v1/alerts/{alert.id}/acknowledge/")
        assert resp.status_code == 200
        assert resp.data["data"]["status"] == "acknowledged"
        assert resp.data["data"]["acknowledged_by"] is not None

    def test_resolve_acknowledged_alert(self, operator_client, org, operator_user):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(rule=rule, camera=camera, organization=org, status="acknowledged")

        resp = operator_client.post(f"/api/v1/alerts/{alert.id}/resolve/")
        assert resp.status_code == 200
        assert resp.data["data"]["status"] == "resolved"
        assert resp.data["data"]["resolved_by"] is not None

    def test_cannot_acknowledge_non_pending(self, operator_client, org):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(rule=rule, camera=camera, organization=org, status="acknowledged")

        resp = operator_client.post(f"/api/v1/alerts/{alert.id}/acknowledge/")
        assert resp.status_code == 400
        assert resp.data["code"] == "INVALID_STATE_TRANSITION"

    def test_cannot_resolve_already_resolved(self, operator_client, org):
        camera = CameraFactory(organization=org)
        rule = AlertRuleFactory(organization=org)
        alert = AlertFactory(rule=rule, camera=camera, organization=org, status="resolved")

        resp = operator_client.post(f"/api/v1/alerts/{alert.id}/resolve/")
        assert resp.status_code == 400
