import uuid

from django.db import migrations, models
import django.db.models.deletion


ALL_GROUP_ID = uuid.UUID("00000000-0000-4000-8000-000000000001")


def seed_all_group(apps, schema_editor):
    group_model = apps.get_model("streams", "Group")
    group_model.objects.update_or_create(
        id=ALL_GROUP_ID,
        defaults={"name": "All", "parent_id": None},
    )


def unseed_all_group(apps, schema_editor):
    group_model = apps.get_model("streams", "Group")
    group_model.objects.filter(id=ALL_GROUP_ID).delete()


class Migration(migrations.Migration):
    initial = True

    dependencies = []

    operations = [
        migrations.CreateModel(
            name="Group",
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
                    "parent",
                    models.ForeignKey(
                        blank=True,
                        null=True,
                        on_delete=django.db.models.deletion.CASCADE,
                        related_name="children",
                        to="streams.group",
                    ),
                ),
            ],
            options={
                "db_table": "streams_group",
                "default_permissions": (),
            },
        ),
        migrations.CreateModel(
            name="Stream",
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
                ("url", models.TextField()),
                ("resolution", models.CharField(blank=True, max_length=64, null=True)),
                ("fps", models.IntegerField(blank=True, null=True)),
                (
                    "status",
                    models.CharField(
                        choices=[("online", "Online"), ("offline", "Offline")],
                        default="offline",
                        max_length=16,
                    ),
                ),
                ("enabled", models.BooleanField(default=True)),
                ("recording", models.BooleanField(default=False)),
                ("last_probe_at", models.DateTimeField(blank=True, null=True)),
                (
                    "group",
                    models.ForeignKey(
                        on_delete=django.db.models.deletion.PROTECT,
                        related_name="streams",
                        to="streams.group",
                    ),
                ),
            ],
            options={
                "db_table": "streams_stream",
            },
        ),
        migrations.AddIndex(
            model_name="stream",
            index=models.Index(fields=["group"], name="streams_str_group_i_idx"),
        ),
        migrations.RunPython(seed_all_group, unseed_all_group),
    ]
