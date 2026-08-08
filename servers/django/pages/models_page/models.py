import uuid

from django.db import models

STATUS_NOT_BUILT = "not_built"
STATUS_BUILDING = "building"
STATUS_BUILT = "built"
STATUS_CHOICES = (
    (STATUS_NOT_BUILT, "Not Built"),
    (STATUS_BUILDING, "Building"),
    (STATUS_BUILT, "Built"),
)

BATCH_MODE_STATIC = "static"
BATCH_MODE_DYNAMIC = "dynamic"
BATCH_MODE_CHOICES = (
    (BATCH_MODE_STATIC, "Static"),
    (BATCH_MODE_DYNAMIC, "Dynamic"),
)

PRECISION_FP16 = "fp16"
PRECISION_CHOICES = ((PRECISION_FP16, "FP16"),)

DEFAULT_OPTIMIZATION_LEVEL = 3
OPTIMIZATION_LEVEL_CHOICES = tuple((level, str(level)) for level in range(6))

DEFAULT_CONF = 0.25
DEFAULT_IOU = 0.45


class MlModel(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    family = models.CharField(max_length=64, default="yolo11")
    version = models.CharField(max_length=64, null=True, blank=True)
    batch_size = models.PositiveIntegerField()
    batch_mode = models.CharField(max_length=16, choices=BATCH_MODE_CHOICES)
    precision = models.CharField(
        max_length=16,
        choices=PRECISION_CHOICES,
        default=PRECISION_FP16,
    )
    optimization_level = models.PositiveSmallIntegerField(
        choices=OPTIMIZATION_LEVEL_CHOICES,
        default=DEFAULT_OPTIMIZATION_LEVEL,
    )
    conf = models.FloatField(default=DEFAULT_CONF)
    iou = models.FloatField(default=DEFAULT_IOU)
    task = models.CharField(max_length=32, null=True, blank=True)
    num_class = models.PositiveIntegerField(null=True, blank=True)
    classes = models.JSONField(null=True, blank=True)
    source_file = models.CharField(max_length=512, null=True, blank=True)
    source_path = models.CharField(max_length=1024, null=True, blank=True)
    engine_path = models.CharField(max_length=1024, null=True, blank=True)
    status = models.CharField(
        max_length=16,
        choices=STATUS_CHOICES,
        default=STATUS_NOT_BUILT,
    )
    last_build_at = models.DateTimeField(null=True, blank=True)
    had_successful_build = models.BooleanField(default=False)
    last_build_error = models.TextField(null=True, blank=True)

    class Meta:
        db_table = "models_mlmodel"
        default_permissions = ()
        permissions = (
            ("view_model", "Can view model"),
            ("add_model", "Can add model"),
            ("change_model", "Can change model"),
            ("delete_model", "Can delete model"),
        )
        ordering = ["name"]
