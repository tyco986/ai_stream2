from rest_framework import serializers

from .models import AnalyticsZone, Camera, CameraGroup


class CameraGroupSerializer(serializers.ModelSerializer):
    class Meta:
        model = CameraGroup
        fields = ["id", "name", "organization", "description", "created_at", "updated_at"]
        read_only_fields = ["id", "organization", "created_at", "updated_at"]

    def create(self, validated_data):
        validated_data["organization"] = self.context["request"].user.organization
        return super().create(validated_data)


class CameraReadSerializer(serializers.ModelSerializer):
    status_display = serializers.CharField(source="get_status_display", read_only=True)
    group_name = serializers.CharField(source="group.name", read_only=True, default=None)

    class Meta:
        model = Camera
        fields = [
            "id", "uid", "name", "rtsp_url", "organization",
            "group", "group_name", "status", "status_display",
            "config", "created_at", "updated_at",
        ]


class CameraWriteSerializer(serializers.ModelSerializer):
    class Meta:
        model = Camera
        fields = ["uid", "name", "rtsp_url", "group", "config"]

    def create(self, validated_data):
        validated_data["organization"] = self.context["request"].user.organization
        return super().create(validated_data)


class AnalyticsZoneSerializer(serializers.ModelSerializer):
    class Meta:
        model = AnalyticsZone
        fields = [
            "id", "camera", "name", "zone_type",
            "coordinates", "config", "is_enabled",
            "created_at", "updated_at",
        ]
        read_only_fields = ["id", "camera", "created_at", "updated_at"]

    def validate_coordinates(self, value):
        if not isinstance(value, list) or len(value) < 2:
            raise serializers.ValidationError("坐标至少需要 2 个点")
        for point in value:
            if not isinstance(point, list) or len(point) != 2:
                raise serializers.ValidationError("每个点必须是 [x, y] 格式")
            x, y = point
            if not (0 <= x <= 1920 and 0 <= y <= 1080):
                raise serializers.ValidationError(
                    f"坐标 ({x}, {y}) 超出范围 [0,1920]x[0,1080]"
                )
        return value

    def validate(self, attrs):
        zone_type = attrs.get("zone_type") or (self.instance and self.instance.zone_type)
        config = attrs.get("config", {})

        if zone_type == "overcrowding" and "object_threshold" not in config:
            raise serializers.ValidationError(
                {"config": "overcrowding 类型必须包含 object_threshold"}
            )
        return attrs
