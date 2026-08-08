import uuid

from django.db import models

ALL_GROUP_ID = uuid.UUID("00000000-0000-4000-8000-000000000001")
ALL_GROUP_NAME = "All"

STREAM_STATUS_ONLINE = "online"
STREAM_STATUS_OFFLINE = "offline"
STREAM_STATUS_CHOICES = (
    (STREAM_STATUS_ONLINE, "Online"),
    (STREAM_STATUS_OFFLINE, "Offline"),
)


class Group(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    parent = models.ForeignKey(
        "self",
        null=True,
        blank=True,
        on_delete=models.CASCADE,
        related_name="children",
    )

    class Meta:
        db_table = "streams_group"
        default_permissions = ()


class Stream(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    group = models.ForeignKey(
        Group,
        on_delete=models.PROTECT,
        related_name="streams",
    )
    url = models.TextField(unique=True, null=True, blank=True)
    resolution = models.CharField(max_length=64, null=True, blank=True)
    fps = models.IntegerField(null=True, blank=True)
    status = models.CharField(
        max_length=16,
        choices=STREAM_STATUS_CHOICES,
        default=STREAM_STATUS_OFFLINE,
    )
    enabled = models.BooleanField(default=True)
    recording = models.BooleanField(default=False)
    last_probe_at = models.DateTimeField(null=True, blank=True)

    class Meta:
        db_table = "streams_stream"
        indexes = [
            models.Index(fields=["group"]),
        ]
