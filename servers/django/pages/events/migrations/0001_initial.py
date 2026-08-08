from django.db import migrations, models
import uuid


class Migration(migrations.Migration):
    initial = True

    dependencies = []

    operations = [
        migrations.CreateModel(
            name="Event",
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
                ("occurred_at", models.DateTimeField(db_index=True)),
                ("stream_id", models.UUIDField(db_index=True)),
                ("stream_name", models.CharField(max_length=255)),
                ("pipeline_id", models.UUIDField(db_index=True)),
                ("pipeline_name", models.CharField(max_length=255)),
                ("event_code", models.CharField(db_index=True, max_length=64)),
                ("event_label", models.CharField(max_length=255)),
                (
                    "status",
                    models.CharField(
                        choices=[("new", "New"), ("acked", "Acked")],
                        db_index=True,
                        default="new",
                        max_length=16,
                    ),
                ),
                (
                    "raw_path",
                    models.CharField(blank=True, max_length=1024, null=True),
                ),
                (
                    "visualization_path",
                    models.CharField(blank=True, max_length=1024, null=True),
                ),
                ("payload", models.JSONField(blank=True, null=True)),
            ],
            options={
                "db_table": "events_event",
                "ordering": ["-occurred_at", "-id"],
                "default_permissions": (),
                "indexes": [
                    models.Index(
                        fields=["-occurred_at", "-id"],
                        name="events_even_occurre_idx",
                    ),
                    models.Index(
                        fields=["pipeline_id", "event_code"],
                        name="events_even_pipelin_idx",
                    ),
                ],
            },
        ),
    ]
