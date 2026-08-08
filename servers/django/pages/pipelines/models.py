import uuid

from django.db import models

PIPELINE_STATUS_STOPPED = "stopped"
PIPELINE_STATUS_STARTING = "starting"
PIPELINE_STATUS_ONLINE = "online"
PIPELINE_STATUS_RUNNING = "running"
PIPELINE_STATUS_STOPPING = "stopping"
PIPELINE_STATUS_ERROR = "error"
PIPELINE_STATUS_OFFLINE = "offline"
PIPELINE_STATUS_CHOICES = (
    (PIPELINE_STATUS_STOPPED, "Stopped"),
    (PIPELINE_STATUS_STARTING, "Starting"),
    (PIPELINE_STATUS_ONLINE, "Online"),
    (PIPELINE_STATUS_RUNNING, "Running"),
    (PIPELINE_STATUS_STOPPING, "Stopping"),
    (PIPELINE_STATUS_ERROR, "Error"),
    (PIPELINE_STATUS_OFFLINE, "Offline"),
)

SOURCE_KIND_FILE = "file"
SOURCE_KIND_STREAM = "stream"
SOURCE_KIND_CHOICES = (
    (SOURCE_KIND_FILE, "File"),
    (SOURCE_KIND_STREAM, "Stream"),
)


class Pipeline(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    type = models.CharField(max_length=64)
    status = models.CharField(
        max_length=16,
        choices=PIPELINE_STATUS_CHOICES,
        default=PIPELINE_STATUS_STOPPED,
    )
    config = models.JSONField(default=dict)
    gie_id = models.UUIDField(null=True, blank=True, db_index=True)
    host_port = models.IntegerField(null=True, blank=True, unique=True)
    status_message = models.TextField(blank=True, default="")
    last_refresh_at = models.DateTimeField(null=True, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    class Meta:
        db_table = "pipelines_pipeline"
        default_permissions = ()
        permissions = (
            ("view_pipeline", "Can view pipeline"),
            ("add_pipeline", "Can add pipeline"),
            ("change_pipeline", "Can change pipeline"),
            ("delete_pipeline", "Can delete pipeline"),
        )
        ordering = ["name"]


class GieTemplate(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    model_id = models.UUIDField()
    model_name = models.CharField(max_length=150, null=True, blank=True)
    class_attrs = models.JSONField(default=list)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    class Meta:
        db_table = "pipelines_gietemplate"
        default_permissions = ()
        ordering = ["name"]


class AnalyzerTemplate(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    source_kind = models.CharField(max_length=16, choices=SOURCE_KIND_CHOICES)
    source_stream_id = models.UUIDField(null=True, blank=True)
    source_file_name = models.CharField(max_length=255, null=True, blank=True)
    source_image_path = models.CharField(max_length=1024, null=True, blank=True)
    config_width = models.IntegerField(default=1920)
    config_height = models.IntegerField(default=1080)
    captured_at = models.DateTimeField(null=True, blank=True)
    annotations = models.JSONField(default=list)
    updated_at = models.DateTimeField(auto_now=True)

    class Meta:
        db_table = "pipelines_analyzertemplate"
        default_permissions = ()
        ordering = ["name"]
