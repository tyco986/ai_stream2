import uuid

from django.db import models

STATUS_NEW = "new"
STATUS_ACKED = "acked"
STATUS_CHOICES = (
    (STATUS_NEW, "New"),
    (STATUS_ACKED, "Acked"),
)


class Event(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    occurred_at = models.DateTimeField(db_index=True)
    stream_id = models.UUIDField(db_index=True)
    stream_name = models.CharField(max_length=255)
    pipeline_id = models.UUIDField(db_index=True)
    pipeline_name = models.CharField(max_length=255)
    event_code = models.CharField(max_length=64, db_index=True)
    event_label = models.CharField(max_length=255)
    status = models.CharField(
        max_length=16,
        choices=STATUS_CHOICES,
        default=STATUS_NEW,
        db_index=True,
    )
    raw_path = models.CharField(max_length=1024, null=True, blank=True)
    visualization_path = models.CharField(max_length=1024, null=True, blank=True)
    payload = models.JSONField(null=True, blank=True)

    class Meta:
        db_table = "events_event"
        default_permissions = ()
        ordering = ["-occurred_at", "-id"]
        indexes = [
            models.Index(fields=["-occurred_at", "-id"]),
            models.Index(fields=["pipeline_id", "event_code"]),
        ]
