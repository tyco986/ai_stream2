from rest_framework import serializers

from .models import Alert, AlertRule


class AlertRuleSerializer(serializers.ModelSerializer):
    rule_type_display = serializers.CharField(source="get_rule_type_display", read_only=True)

    class Meta:
        model = AlertRule
        fields = [
            "id", "name", "organization", "rule_type", "rule_type_display",
            "conditions", "cameras", "is_enabled", "cooldown_seconds",
            "notify_channels", "created_at", "updated_at",
        ]
        read_only_fields = ["id", "organization", "created_at", "updated_at"]

    def create(self, validated_data):
        cameras = validated_data.pop("cameras", [])
        validated_data["organization"] = self.context["request"].user.organization
        rule = AlertRule.objects.create(**validated_data)
        if cameras:
            rule.cameras.set(cameras)
        return rule


class AlertSerializer(serializers.ModelSerializer):
    rule_name = serializers.CharField(source="rule.name", read_only=True)
    camera_uid = serializers.CharField(source="camera.uid", read_only=True)
    status_display = serializers.CharField(source="get_status_display", read_only=True)

    class Meta:
        model = Alert
        fields = [
            "id", "rule", "rule_name", "camera", "camera_uid",
            "organization", "triggered_at", "status", "status_display",
            "snapshot", "acknowledged_by", "acknowledged_at",
            "resolved_by", "resolved_at", "created_at",
        ]
        read_only_fields = fields
