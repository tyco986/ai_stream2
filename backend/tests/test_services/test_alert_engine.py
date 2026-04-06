"""AlertEngine unit tests — rule matching, cooldown, analytics rules."""
from datetime import timedelta
from unittest.mock import patch

import pytest
from django.utils.timezone import now

from services.alert_engine import AlertEngine
from tests.factories import (
    AlertRuleFactory,
    CameraFactory,
    DetectionFactory,
    OrganizationFactory,
)


@pytest.fixture
def org():
    return OrganizationFactory()


@pytest.fixture
def camera(org):
    return CameraFactory(organization=org)


@pytest.fixture
def engine():
    return AlertEngine()


# ---- object_count rule ----


@pytest.mark.django_db
class TestObjectCountRule:
    def test_triggers_when_above_threshold(self, engine, camera):
        rule = AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_count",
            conditions={"min_count": 5},
        )
        detection = DetectionFactory.build(camera=camera, object_count=7)
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 1
        assert alerts[0].rule == rule

    def test_does_not_trigger_below_threshold(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_count",
            conditions={"min_count": 5},
        )
        detection = DetectionFactory.build(camera=camera, object_count=3)
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 0

    def test_cold_start_triggers_on_first_detection(self, camera):
        engine = AlertEngine()
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_count",
            conditions={"min_count": 1},
        )
        detection = DetectionFactory.build(camera=camera, object_count=2)
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 1


# ---- cooldown ----


@pytest.mark.django_db
class TestCooldown:
    def test_respects_cooldown_period(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_count",
            conditions={"min_count": 1},
            cooldown_seconds=300,
        )
        det1 = DetectionFactory.build(camera=camera, object_count=5)
        det2 = DetectionFactory.build(camera=camera, object_count=5)

        alerts1 = engine.evaluate_detection(det1)
        alerts2 = engine.evaluate_detection(det2)
        assert len(alerts1) == 1
        assert len(alerts2) == 0

    def test_triggers_after_cooldown_expires(self, engine, camera):
        rule = AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_count",
            conditions={"min_count": 1},
            cooldown_seconds=60,
        )
        det1 = DetectionFactory.build(camera=camera, object_count=5)
        engine.evaluate_detection(det1)

        engine._last_triggered[(rule.id, camera.id)] = now() - timedelta(seconds=61)

        det2 = DetectionFactory.build(camera=camera, object_count=5)
        alerts = engine.evaluate_detection(det2)
        assert len(alerts) == 1


# ---- object_type rule ----


@pytest.mark.django_db
class TestObjectTypeRule:
    def test_triggers_for_matching_type(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_type",
            conditions={"object_type": "person", "min_count": 2},
        )
        detection = DetectionFactory.build(
            camera=camera,
            detected_objects=[
                {"type": "person", "confidence": 0.9},
                {"type": "person", "confidence": 0.8},
                {"type": "car", "confidence": 0.7},
            ],
            object_count=3,
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 1

    def test_no_trigger_insufficient_count(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="object_type",
            conditions={"object_type": "person", "min_count": 5},
        )
        detection = DetectionFactory.build(
            camera=camera,
            detected_objects=[{"type": "person", "confidence": 0.9}],
            object_count=1,
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 0


# ---- zone_intrusion rule ----


@pytest.mark.django_db
class TestZoneIntrusionRule:
    def test_triggers_on_roi_status(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="zone_intrusion",
            conditions={"zone_name": "restricted_area"},
        )
        detection = DetectionFactory.build(
            camera=camera,
            detected_objects=[
                {
                    "type": "person",
                    "analytics": {"roiStatus": ["restricted_area"]},
                },
            ],
            object_count=1,
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 1

    def test_no_trigger_without_roi(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="zone_intrusion",
            conditions={"zone_name": "restricted_area"},
        )
        detection = DetectionFactory.build(
            camera=camera,
            detected_objects=[{"type": "person", "analytics": {"roiStatus": []}}],
            object_count=1,
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 0


# ---- line_crossing rule ----


@pytest.mark.django_db
class TestLineCrossingRule:
    def test_triggers_above_threshold(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="line_crossing",
            conditions={"line_name": "gate-1", "min_count": 3},
        )
        detection = DetectionFactory.build(
            camera=camera,
            object_count=1,
            analytics={
                "lineCrossing": [{"name": "gate-1", "in": 2, "out": 2}],
            },
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 1

    def test_no_trigger_below_threshold(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="line_crossing",
            conditions={"line_name": "gate-1", "min_count": 10},
        )
        detection = DetectionFactory.build(
            camera=camera,
            object_count=1,
            analytics={
                "lineCrossing": [{"name": "gate-1", "in": 1, "out": 0}],
            },
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 0

    def test_no_trigger_without_analytics(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="line_crossing",
            conditions={"line_name": "gate-1", "min_count": 1},
        )
        detection = DetectionFactory.build(camera=camera, analytics=None, object_count=1)
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 0


# ---- overcrowding rule ----


@pytest.mark.django_db
class TestOvercrowdingRule:
    def test_triggers_when_overcrowded(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="overcrowding",
            conditions={"zone_name": "lobby"},
        )
        detection = DetectionFactory.build(
            camera=camera,
            object_count=1,
            analytics={
                "overcrowding": {
                    "roi_name": "lobby",
                    "triggered": True,
                    "count": 12,
                },
            },
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 1

    def test_no_trigger_when_not_overcrowded(self, engine, camera):
        AlertRuleFactory(
            organization=camera.organization,
            rule_type="overcrowding",
            conditions={"zone_name": "lobby"},
        )
        detection = DetectionFactory.build(
            camera=camera,
            object_count=1,
            analytics={
                "overcrowding": {
                    "roi_name": "lobby",
                    "triggered": False,
                    "count": 2,
                },
            },
        )
        alerts = engine.evaluate_detection(detection)
        assert len(alerts) == 0


# ---- prune ----


@pytest.mark.django_db
class TestCacheManagement:
    def test_prune_removes_old_entries(self, engine, camera):
        rule = AlertRuleFactory(organization=camera.organization, rule_type="object_count", conditions={"min_count": 1})
        engine._last_triggered[(rule.id, camera.id)] = now() - timedelta(days=2)
        engine._cache_ttl_seconds = 3600
        engine.prune_cooldown_cache()
        assert (rule.id, camera.id) not in engine._last_triggered
