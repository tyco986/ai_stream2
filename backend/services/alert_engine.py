import time

import structlog
from django.utils.timezone import now

logger = structlog.get_logger(__name__)


class AlertEngine:
    """规则引擎：内存缓存冷却状态，避免热路径上的 DB 查询。

    初版支持检测规则和分析规则（nvdsanalytics）。
    """

    DETECTION_RULE_TYPES = {"object_count", "object_type"}
    ANALYTICS_RULE_TYPES = {"zone_intrusion", "line_crossing", "overcrowding"}

    def __init__(self):
        self._last_triggered = {}   # (rule_id, camera_id) → datetime
        self._cache_ttl_seconds = 86400
        self._rules_cache = []
        self._rules_cache_loaded_at = 0
        self._rules_cache_ttl = 30

    def _load_rules(self):
        """Load active rules from DB with TTL cache."""
        current = time.time()
        if current - self._rules_cache_loaded_at > self._rules_cache_ttl:
            # 避免循环导入
            from apps.alerts.models import AlertRule  # noqa: E402 — 解决循环依赖
            self._rules_cache = list(
                AlertRule.objects.filter(is_enabled=True).prefetch_related("cameras")
            )
            self._rules_cache_loaded_at = current
        return self._rules_cache

    def evaluate_detection(self, detection):
        """评估 Detection 触发的规则。返回触发的 Alert 列表。"""
        active_rules = self._load_rules()
        rule_types = self.DETECTION_RULE_TYPES | self.ANALYTICS_RULE_TYPES
        rules = [r for r in active_rules if r.rule_type in rule_types]
        return self._evaluate(detection, rules)

    def _evaluate(self, record, rules):
        triggered_alerts = []
        for rule in rules:
            if not self._camera_matches(record.camera, rule):
                continue
            if not self._cooldown_passed(rule, record.camera):
                continue
            if self._conditions_match(record, rule):
                alert = self._create_alert(record, rule)
                triggered_alerts.append(alert)
                self._last_triggered[(rule.id, record.camera_id)] = now()
        return triggered_alerts

    def _camera_matches(self, camera, rule):
        """空 cameras M2M = 匹配全部摄像头。"""
        if not rule.cameras.exists():
            return True
        return rule.cameras.filter(id=camera.id).exists()

    def _cooldown_passed(self, rule, camera):
        """内存缓存冷却判断，O(1) 查找，不查数据库。"""
        key = (rule.id, camera.id)
        last_time = self._last_triggered.get(key)
        if not last_time:
            return True
        elapsed = (now() - last_time).total_seconds()
        return elapsed >= rule.cooldown_seconds

    def _conditions_match(self, detection, rule):
        conditions = rule.conditions or {}
        rule_type = rule.rule_type

        if rule_type == "object_count":
            min_count = conditions.get("min_count", 1)
            return detection.object_count >= min_count

        if rule_type == "object_type":
            target_type = conditions.get("object_type")
            min_count = conditions.get("min_count", 1)
            if not target_type:
                return False
            matched = [
                o for o in (detection.detected_objects or [])
                if o.get("type") == target_type
            ]
            return len(matched) >= min_count

        if rule_type == "zone_intrusion":
            zone_name = conditions.get("zone_name")
            object_type = conditions.get("object_type")
            for obj in (detection.detected_objects or []):
                obj_analytics = obj.get("analytics", {})
                roi_status = obj_analytics.get("roiStatus", [])
                if zone_name and zone_name in roi_status:
                    if object_type is None or obj.get("type") == object_type:
                        return True
            return False

        if rule_type == "line_crossing":
            analytics = detection.analytics
            if not analytics:
                return False
            line_name = conditions.get("line_name")
            min_count = conditions.get("min_count", 1)
            for lc in analytics.get("lineCrossing", []):
                if lc.get("name") == line_name:
                    total = lc.get("in", 0) + lc.get("out", 0)
                    if total >= min_count:
                        return True
            return False

        if rule_type == "overcrowding":
            analytics = detection.analytics
            if not analytics:
                return False
            oc = analytics.get("overcrowding", {})
            zone_name = conditions.get("zone_name")
            if zone_name and oc.get("roi_name") == zone_name:
                return oc.get("triggered", False)
            return False

        return False

    def _create_alert(self, detection, rule):
        from apps.alerts.models import Alert  # noqa: E402 — 解决循环依赖
        return Alert(
            rule=rule,
            camera=detection.camera,
            organization=detection.camera.organization,
            triggered_at=detection.detected_at,
            snapshot={
                "detection_id": detection.id,
                "object_count": detection.object_count,
                "objects": detection.detected_objects[:5] if detection.detected_objects else [],
                "analytics": detection.analytics,
            },
        )

    def prune_cooldown_cache(self):
        """清理过旧的冷却 key，避免内存字典无限增长。"""
        from datetime import timedelta
        threshold = now() - timedelta(seconds=self._cache_ttl_seconds)
        self._last_triggered = {
            key: ts for key, ts in self._last_triggered.items()
            if ts >= threshold
        }
