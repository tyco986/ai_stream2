import uuid

from django.conf import settings
from django.db import models

DEFAULT_LAYOUT_ID = uuid.UUID("00000000-0000-4000-8000-000000000002")
DEFAULT_LAYOUT_NAME = "Default"

LAYOUT_1X1 = "1x1"
LAYOUT_2X2 = "2x2"
LAYOUT_3X3 = "3x3"
LAYOUT_4X4 = "4x4"
LAYOUT_CHOICES = (
    (LAYOUT_1X1, "1x1"),
    (LAYOUT_2X2, "2x2"),
    (LAYOUT_3X3, "3x3"),
    (LAYOUT_4X4, "4x4"),
)
LAYOUT_SLOT_COUNT = {
    LAYOUT_1X1: 1,
    LAYOUT_2X2: 4,
    LAYOUT_3X3: 9,
    LAYOUT_4X4: 16,
}

VIEW_MODE_GRID = "grid"
VIEW_MODE_FOCUS = "focus"
VIEW_MODE_CHOICES = (
    (VIEW_MODE_GRID, "Grid"),
    (VIEW_MODE_FOCUS, "Focus"),
)


class LayoutPreset(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=150, unique=True)
    layout = models.CharField(max_length=8, choices=LAYOUT_CHOICES)
    view_mode = models.CharField(
        max_length=16,
        choices=VIEW_MODE_CHOICES,
        default=VIEW_MODE_GRID,
    )
    slots = models.JSONField(default=list)
    shot_path = models.CharField(max_length=512, null=True, blank=True)

    class Meta:
        db_table = "preview_layoutpreset"
        default_permissions = ()
        permissions = (
            ("view_preview", "Can view preview"),
            ("add_preview", "Can add preview"),
            ("change_preview", "Can change preview"),
            ("delete_preview", "Can delete preview"),
        )


class ActiveLayout(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    user = models.OneToOneField(
        settings.AUTH_USER_MODEL,
        on_delete=models.CASCADE,
        related_name="preview_active_layout",
    )
    preset_id = models.UUIDField()

    class Meta:
        db_table = "preview_activelayout"
        default_permissions = ()
