from rest_framework import serializers

from pages.preview.models import LAYOUT_CHOICES, VIEW_MODE_CHOICES


class ActiveLayoutSerializer(serializers.Serializer):
    preset_id = serializers.UUIDField()


class CreateLayoutSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    layout = serializers.ChoiceField(choices=[item[0] for item in LAYOUT_CHOICES])
    view_mode = serializers.ChoiceField(choices=[item[0] for item in VIEW_MODE_CHOICES])
    slots = serializers.ListField(allow_empty=True)


class PatchLayoutSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150, required=False)
    layout = serializers.ChoiceField(
        choices=[item[0] for item in LAYOUT_CHOICES],
        required=False,
    )
    view_mode = serializers.ChoiceField(
        choices=[item[0] for item in VIEW_MODE_CHOICES],
        required=False,
    )
    slots = serializers.ListField(required=False, allow_empty=True)


class BatchDeleteLayoutsSerializer(serializers.Serializer):
    ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )
