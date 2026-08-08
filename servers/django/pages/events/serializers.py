from rest_framework import serializers


class AckActionSerializer(serializers.Serializer):
    action = serializers.ChoiceField(choices=["ack", "unack"])


class BatchAckSerializer(serializers.Serializer):
    event_ids = serializers.ListField(
        child=serializers.UUIDField(),
        allow_empty=True,
    )
    action = serializers.ChoiceField(choices=["ack", "unack"])


class CollectSerializer(serializers.Serializer):
    passphrase = serializers.CharField(min_length=1, allow_blank=False)
