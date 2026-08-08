from rest_framework import serializers

from pages.shell.models import VERSION_PATTERN


class PageSettingsSerializer(serializers.Serializer):
    mode = serializers.ChoiceField(choices=["sidebar", "topbar"])


class SiteConfigVersionCreateSerializer(serializers.Serializer):
    version = serializers.CharField(max_length=64)
    description = serializers.CharField(
        required=False, allow_blank=True, allow_null=True, default=None
    )

    def validate_version(self, value):
        if not VERSION_PATTERN.match(value):
            raise serializers.ValidationError("Invalid version format")
        return value


class SiteConfigVersionSerializer(serializers.Serializer):
    id = serializers.UUIDField()
    version = serializers.CharField()
    description = serializers.CharField(allow_null=True)
    created_at = serializers.DateTimeField()
    is_current = serializers.BooleanField()
