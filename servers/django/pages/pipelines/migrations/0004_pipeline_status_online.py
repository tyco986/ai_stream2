from django.db import migrations, models


def forwards_running_to_online(apps, schema_editor):
    Pipeline = apps.get_model("pipelines", "Pipeline")
    Pipeline.objects.filter(status="running").update(status="online")


def backwards_online_to_running(apps, schema_editor):
    Pipeline = apps.get_model("pipelines", "Pipeline")
    Pipeline.objects.filter(status="online").update(status="running")


class Migration(migrations.Migration):
    dependencies = [
        ("pipelines", "0003_pipeline_last_refresh_at"),
    ]

    operations = [
        migrations.RunPython(forwards_running_to_online, backwards_online_to_running),
        migrations.AlterField(
            model_name="pipeline",
            name="status",
            field=models.CharField(
                choices=[
                    ("stopped", "Stopped"),
                    ("starting", "Starting"),
                    ("online", "Online"),
                    ("stopping", "Stopping"),
                    ("error", "Error"),
                    ("offline", "Offline"),
                ],
                default="stopped",
                max_length=16,
            ),
        ),
    ]
