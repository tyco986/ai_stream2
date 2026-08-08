from rest_framework import serializers


class CreateGroupSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    permission_ids = serializers.ListField(
        child=serializers.IntegerField(min_value=1),
        required=False,
        allow_empty=True,
    )


class PatchGroupSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150, required=False)
    permission_ids = serializers.ListField(
        child=serializers.IntegerField(min_value=1),
        required=False,
        allow_empty=True,
    )


class CreateUserSerializer(serializers.Serializer):
    username = serializers.CharField(max_length=150)
    password = serializers.CharField(allow_blank=False)
    is_active = serializers.BooleanField(required=False, default=True)
    group_ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=False,
    )


class PatchUserSerializer(serializers.Serializer):
    username = serializers.CharField(max_length=150, required=False)
    password = serializers.CharField(required=False, allow_blank=False)
    is_active = serializers.BooleanField(required=False)
    group_ids = serializers.ListField(
        child=serializers.UUIDField(),
        required=False,
        allow_empty=False,
    )


class BatchDeleteUsersSerializer(serializers.Serializer):
    user_ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )
