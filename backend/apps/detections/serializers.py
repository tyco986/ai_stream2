from rest_framework import serializers

from .models import Detection


class DetectionSerializer(serializers.ModelSerializer):
    camera_uid = serializers.CharField(source="camera.uid", read_only=True)

    class Meta:
        model = Detection
        fields = [
            "id", "camera", "camera_uid", "detected_at", "ingested_at",
            "frame_number", "object_count", "detected_objects", "analytics",
        ]
