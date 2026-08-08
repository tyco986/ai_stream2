import uuid

from django.db import migrations, models


class Migration(migrations.Migration):
    initial = True

    dependencies = []

    operations = [
        migrations.CreateModel(
            name="Pipeline",
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
                ("type", models.CharField(max_length=64)),
                (
                    "status",
                    models.CharField(
                        choices=[
                            ("stopped", "Stopped"),
                            ("starting", "Starting"),
                            ("running", "Running"),
                            ("stopping", "Stopping"),
                            ("error", "Error"),
                        ],
                        default="stopped",
                        max_length=16,
                    ),
                ),
                ("config", models.JSONField(default=dict)),
                ("gie_id", models.UUIDField(blank=True, db_index=True, null=True)),
                ("status_message", models.TextField(blank=True, default="")),
                ("created_at", models.DateTimeField(auto_now_add=True)),
                ("updated_at", models.DateTimeField(auto_now=True)),
            ],
            options={
                "db_table": "pipelines_pipeline",
                "ordering": ["name"],
                "default_permissions": (),
                "permissions": (
                    ("view_pipeline", "Can view pipeline"),
                    ("add_pipeline", "Can add pipeline"),
                    ("change_pipeline", "Can change pipeline"),
                    ("delete_pipeline", "Can delete pipeline"),
                ),
            },
        ),
        migrations.CreateModel(
            name="GieTemplate",
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
                ("model_id", models.UUIDField()),
                (
                    "model_name",
                    models.CharField(blank=True, max_length=150, null=True),
                ),
                ("class_attrs", models.JSONField(default=list)),
                ("created_at", models.DateTimeField(auto_now_add=True)),
                ("updated_at", models.DateTimeField(auto_now=True)),
            ],
            options={
                "db_table": "pipelines_gietemplate",
                "ordering": ["name"],
                "default_permissions": (),
            },
        ),
        migrations.CreateModel(
            name="AnalyzerTemplate",
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
                    "source_kind",
                    models.CharField(
                        choices=[("file", "File"), ("stream", "Stream")],
                        max_length=16,
                    ),
                ),
                ("source_stream_id", models.UUIDField(blank=True, null=True)),
                (
                    "source_file_name",
                    models.CharField(blank=True, max_length=255, null=True),
                ),
                (
                    "source_image_path",
                    models.CharField(blank=True, max_length=1024, null=True),
                ),
                ("config_width", models.IntegerField(default=1920)),
                ("config_height", models.IntegerField(default=1080)),
                ("captured_at", models.DateTimeField(blank=True, null=True)),
                ("annotations", models.JSONField(default=list)),
                ("updated_at", models.DateTimeField(auto_now=True)),
            ],
            options={
                "db_table": "pipelines_analyzertemplate",
                "ordering": ["name"],
                "default_permissions": (),
            },
        ),
    ]
