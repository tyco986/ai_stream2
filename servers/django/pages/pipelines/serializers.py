from rest_framework import serializers


class PipelineSchemaSerializer(serializers.Serializer):
    pipeline_type = serializers.CharField(max_length=64)


class BatchIdsSerializer(serializers.Serializer):
    ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )


class PipelineBodySerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    type = serializers.CharField(max_length=64)
    drawer = serializers.JSONField(required=False)
    parser = serializers.JSONField(required=False)
    logger = serializers.JSONField(required=False)
    messager = serializers.JSONField(required=False)
    debouncer = serializers.JSONField(required=False, allow_null=True)
    streams = serializers.ListField(
        child=serializers.UUIDField(),
        required=False,
        allow_empty=True,
    )
    gie_id = serializers.UUIDField()
    interval = serializers.IntegerField(min_value=0)
    tracker = serializers.JSONField(required=False, allow_null=True)
    analyzer = serializers.JSONField(required=False, allow_null=True)
    sahi = serializers.JSONField(required=False, allow_null=True)
    input = serializers.CharField(
        required=False,
        allow_null=True,
        allow_blank=True,
        max_length=1024,
    )
    output = serializers.CharField(
        required=False,
        allow_null=True,
        allow_blank=True,
        max_length=1024,
    )


class GieTemplateBodySerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    model_id = serializers.UUIDField()
    class_attrs = serializers.ListField(child=serializers.JSONField(), min_length=1)


class AnalyzerTemplateBodySerializer(serializers.Serializer):
    name = serializers.CharField(max_length=150)
    source_kind = serializers.ChoiceField(choices=["file", "stream"])
    source_stream_id = serializers.UUIDField(required=False, allow_null=True)
    source_file_name = serializers.CharField(
        max_length=255,
        required=False,
        allow_null=True,
        allow_blank=True,
    )
    config_width = serializers.IntegerField(required=False, min_value=1)
    config_height = serializers.IntegerField(required=False, min_value=1)
    annotations = serializers.ListField(
        child=serializers.JSONField(),
        required=False,
    )


class AnalyzerSourceStreamSerializer(serializers.Serializer):
    stream_id = serializers.UUIDField()
