import uuid

import django.db.models.deletion
from django.conf import settings
from django.db import migrations, models

DEFAULT_LAYOUT_ID = uuid.UUID("00000000-0000-4000-8000-000000000002")


def seed_default_layout(apps, schema_editor):
    layout_preset = apps.get_model("previews", "LayoutPreset")
    layout_preset.objects.update_or_create(
        id=DEFAULT_LAYOUT_ID,
        defaults={
            "name": "Default",
            "layout": "2x2",
            "view_mode": "grid",
            "slots": [None, None, None, None],
            "shot_path": None,
        },
    )


def unseed_default_layout(apps, schema_editor):
    layout_preset = apps.get_model("previews", "LayoutPreset")
    layout_preset.objects.filter(id=DEFAULT_LAYOUT_ID).delete()


class Migration(migrations.Migration):
    initial = True

    dependencies = [
        migrations.swappable_dependency(settings.AUTH_USER_MODEL),
    ]

    operations = [
        migrations.CreateModel(
            name="LayoutPreset",
            fields=[
                (
                    "id",
                    models.UUIDField(
                        default=uuid.uuid4,
                        editable=False,
                        primary_key=True,
                        serialize=False,
                    ),
                ),
                ("name", models.CharField(max_length=150, unique=True)),
                (
                    "layout",
                    models.CharField(
                        choices=[
                            ("1x1", "1x1"),
                            ("2x2", "2x2"),
                            ("3x3", "3x3"),
                            ("4x4", "4x4"),
                        ],
                        max_length=8,
                    ),
                ),
                (
                    "view_mode",
                    models.CharField(
                        choices=[("grid", "Grid"), ("focus", "Focus")],
                        default="grid",
                        max_length=16,
                    ),
                ),
                ("slots", models.JSONField(default=list)),
                ("shot_path", models.CharField(blank=True, max_length=512, null=True)),
            ],
            options={
                "db_table": "preview_layoutpreset",
                "default_permissions": (),
                "permissions": (
                    ("view_preview", "Can view preview"),
                    ("add_preview", "Can add preview"),
                    ("change_preview", "Can change preview"),
                    ("delete_preview", "Can delete preview"),
                ),
            },
        ),
        migrations.CreateModel(
            name="ActiveLayout",
            fields=[
                (
                    "id",
                    models.UUIDField(
                        default=uuid.uuid4,
                        editable=False,
                        primary_key=True,
                        serialize=False,
                    ),
                ),
                ("preset_id", models.UUIDField()),
                (
                    "user",
                    models.OneToOneField(
                        on_delete=django.db.models.deletion.CASCADE,
                        related_name="preview_active_layout",
                        to=settings.AUTH_USER_MODEL,
                    ),
                ),
            ],
            options={
                "db_table": "preview_activelayout",
                "default_permissions": (),
            },
        ),
        migrations.RunPython(seed_default_layout, unseed_default_layout),
    ]
