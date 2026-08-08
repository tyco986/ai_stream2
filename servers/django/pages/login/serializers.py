from rest_framework import serializers


class LoginSerializer(serializers.Serializer):
    username = serializers.CharField(allow_blank=False)
    password = serializers.CharField(allow_blank=False, write_only=True)
    new_password = serializers.CharField(
        required=False,
        allow_blank=False,
        write_only=True,
    )
