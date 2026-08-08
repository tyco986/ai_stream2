from rest_framework import serializers


class GroupNameSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)


class SetMembersSerializer(serializers.Serializer):
    stream_ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )


class CreateStreamSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    url = serializers.CharField(allow_blank=True)
    group_id = serializers.UUIDField(required=False)
    enabled = serializers.BooleanField(required=False, default=True)
    recording = serializers.BooleanField(required=False, default=False)
    resolution = serializers.CharField(required=False, allow_blank=True, allow_null=True)
    fps = serializers.IntegerField(required=False, allow_null=True)


class PatchStreamSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150, required=False)
    url = serializers.CharField(required=False, allow_blank=True)
    group_id = serializers.UUIDField(required=False)
    enabled = serializers.BooleanField(required=False)
    recording = serializers.BooleanField(required=False)
    resolution = serializers.CharField(required=False, allow_blank=True, allow_null=True)
    fps = serializers.IntegerField(required=False, allow_null=True)


class BatchIdsSerializer(serializers.Serializer):
    ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )


class ProbeUrlSerializer(serializers.Serializer):
    url = serializers.CharField(allow_blank=True)
