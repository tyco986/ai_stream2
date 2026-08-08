import uuid

import django.db.models.deletion
from django.conf import settings
from django.db import migrations, models


class Migration(migrations.Migration):
    initial = True

    dependencies = [
        migrations.swappable_dependency(settings.AUTH_USER_MODEL),
    ]

    operations = [
        migrations.CreateModel(
            name="UsedAuthTicket",
            fields=[
                (
                    "id",
                    models.UUIDField(
                        default=uuid.uuid4, editable=False, primary_key=True, serialize=False
                    ),
                ),
                ("jti", models.CharField(max_length=128, unique=True)),
                ("used_at", models.DateTimeField(auto_now_add=True)),
                ("action", models.CharField(max_length=16)),
            ],
            options={
                "db_table": "shell_usedauthticket",
            },
        ),
        migrations.CreateModel(
            name="SiteConfigVersion",
            fields=[
                (
                    "id",
                    models.UUIDField(
                        default=uuid.uuid4, editable=False, primary_key=True, serialize=False
                    ),
                ),
                ("version", models.CharField(max_length=64, unique=True)),
                ("description", models.TextField(blank=True, null=True)),
                ("created_at", models.DateTimeField(auto_now_add=True)),
                ("is_current", models.BooleanField(default=False)),
                ("payload_path", models.TextField()),
            ],
            options={
                "db_table": "shell_siteconfigversion",
                "ordering": ["-created_at"],
            },
        ),
        migrations.CreateModel(
            name="PageSettings",
            fields=[
                (
                    "id",
                    models.UUIDField(
                        default=uuid.uuid4, editable=False, primary_key=True, serialize=False
                    ),
                ),
                ("mode", models.CharField(max_length=16)),
                (
                    "user",
                    models.OneToOneField(
                        on_delete=django.db.models.deletion.CASCADE,
                        related_name="shell_page_settings",
                        to=settings.AUTH_USER_MODEL,
                    ),
                ),
            ],
            options={
                "db_table": "shell_pagesettings",
            },
        ),
    ]
