from rest_framework import serializers

from pages.models_page.models import (
    DEFAULT_CONF,
    DEFAULT_IOU,
    DEFAULT_OPTIMIZATION_LEVEL,
    OPTIMIZATION_LEVEL_CHOICES,
    PRECISION_FP16,
    BATCH_MODE_CHOICES,
)


class BatchDeleteModelsSerializer(serializers.Serializer):
    ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )


class CreateModelSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    family = serializers.CharField(max_length=64)
    batch_mode = serializers.ChoiceField(choices=[item[0] for item in BATCH_MODE_CHOICES])
    batch_size = serializers.IntegerField()
    precision = serializers.CharField(required=False, allow_blank=True)
    optimization_level = serializers.ChoiceField(
        choices=[item[0] for item in OPTIMIZATION_LEVEL_CHOICES],
        required=False,
        default=DEFAULT_OPTIMIZATION_LEVEL,
    )
    conf = serializers.FloatField(required=False, default=DEFAULT_CONF)
    iou = serializers.FloatField(required=False, default=DEFAULT_IOU)

    def validate_family(self, value):
        cleaned = (value or "").strip()
        if not cleaned:
            raise serializers.ValidationError("family is required")
        return cleaned

    def validate_batch_size(self, value):
        if value < 1 or value > 128:
            raise serializers.ValidationError("batch_size must be 1..128")
        return value

    def validate_precision(self, value):
        cleaned = (value or PRECISION_FP16).strip() or PRECISION_FP16
        if cleaned != PRECISION_FP16:
            raise serializers.ValidationError("precision must be fp16")
        return cleaned

    def validate_conf(self, value):
        if not 0.0 < value <= 1.0:
            raise serializers.ValidationError("conf must be in (0, 1]")
        return value

    def validate_iou(self, value):
        if not 0.0 < value <= 1.0:
            raise serializers.ValidationError("iou must be in (0, 1]")
        return value
