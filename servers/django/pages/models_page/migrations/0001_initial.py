import uuid

from django.db import migrations, models


class Migration(migrations.Migration):
    initial = True

    dependencies = []

    operations = [
        migrations.CreateModel(
            name="MlModel",
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
                ("version", models.CharField(blank=True, max_length=64, null=True)),
                ("batch_size", models.PositiveIntegerField()),
                (
                    "batch_mode",
                    models.CharField(
                        choices=[("static", "Static"), ("dynamic", "Dynamic")],
                        max_length=16,
                    ),
                ),
                (
                    "precision",
                    models.CharField(
                        choices=[("fp16", "FP16")],
                        default="fp16",
                        max_length=16,
                    ),
                ),
                ("task", models.CharField(blank=True, max_length=32, null=True)),
                ("num_class", models.PositiveIntegerField(blank=True, null=True)),
                ("classes", models.JSONField(blank=True, null=True)),
                (
                    "source_file",
                    models.CharField(blank=True, max_length=512, null=True),
                ),
                (
                    "source_path",
                    models.CharField(blank=True, max_length=1024, null=True),
                ),
                (
                    "engine_path",
                    models.CharField(blank=True, max_length=1024, null=True),
                ),
                (
                    "status",
                    models.CharField(
                        choices=[
                            ("not_built", "Not Built"),
                            ("building", "Building"),
                            ("built", "Built"),
                        ],
                        default="not_built",
                        max_length=16,
                    ),
                ),
                ("last_build_at", models.DateTimeField(blank=True, null=True)),
                ("had_successful_build", models.BooleanField(default=False)),
                ("last_build_error", models.TextField(blank=True, null=True)),
            ],
            options={
                "db_table": "models_mlmodel",
                "ordering": ["name"],
                "default_permissions": (),
                "permissions": (
                    ("view_model", "Can view model"),
                    ("add_model", "Can add model"),
                    ("change_model", "Can change model"),
                    ("delete_model", "Can delete model"),
                ),
            },
        ),
    ]
