from rest_framework import serializers

from .models import AIModel, CameraModelBinding, PipelineProfile


class AIModelSerializer(serializers.ModelSerializer):
    model_type_display = serializers.CharField(source="get_model_type_display", read_only=True)
    framework_display = serializers.CharField(source="get_framework_display", read_only=True)

    class Meta:
        model = AIModel
        fields = [
            "id", "name", "organization", "model_type", "model_type_display",
            "framework", "framework_display", "model_file", "label_file",
            "config", "version", "description", "is_active",
            "created_at", "updated_at",
        ]
        read_only_fields = ["id", "organization", "created_at", "updated_at"]

    def create(self, validated_data):
        validated_data["organization"] = self.context["request"].user.organization
        return super().create(validated_data)

    def validate(self, attrs):
        model_type = attrs.get("model_type") or (self.instance and self.instance.model_type)
        config = attrs.get("config", {})

        if model_type == "detector":
            required = ["num_classes"]
            for field in required:
                if field not in config:
                    raise serializers.ValidationError(
                        {"config": f"detector 类型必须包含 {field}"}
                    )
        elif model_type == "tracker":
            if "tracker_type" not in config:
                raise serializers.ValidationError(
                    {"config": "tracker 类型必须包含 tracker_type"}
                )
        return attrs


class PipelineProfileSerializer(serializers.ModelSerializer):
    class Meta:
        model = PipelineProfile
        fields = [
            "id", "name", "organization", "description",
            "detector", "tracker", "analytics_enabled",
            "analytics_config_stale", "is_active",
            "created_at", "updated_at",
        ]
        read_only_fields = ["id", "organization", "analytics_config_stale", "created_at", "updated_at"]

    def create(self, validated_data):
        validated_data["organization"] = self.context["request"].user.organization
        return super().create(validated_data)

    def validate_detector(self, value):
        if value.model_type != "detector":
            raise serializers.ValidationError("detector 必须是 detector 类型的 AIModel")
        org = self.context["request"].user.organization
        if value.organization != org:
            raise serializers.ValidationError("不能使用其他组织的模型")
        return value

    def validate_tracker(self, value):
        if value is None:
            return value
        if value.model_type != "tracker":
            raise serializers.ValidationError("tracker 必须是 tracker 类型的 AIModel")
        org = self.context["request"].user.organization
        if value.organization != org:
            raise serializers.ValidationError("不能使用其他组织的模型")
        return value


class CameraModelBindingSerializer(serializers.ModelSerializer):
    camera_uid = serializers.CharField(source="camera.uid", read_only=True)
    pipeline_name = serializers.CharField(source="pipeline_profile.name", read_only=True)

    class Meta:
        model = CameraModelBinding
        fields = [
            "id", "camera", "camera_uid",
            "pipeline_profile", "pipeline_name",
            "is_enabled", "created_at",
        ]
        read_only_fields = ["id", "created_at"]
