from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("pipelines", "0004_pipeline_status_online"),
    ]

    operations = [
        migrations.AlterField(
            model_name="pipeline",
            name="status",
            field=models.CharField(
                choices=[
                    ("stopped", "Stopped"),
                    ("starting", "Starting"),
                    ("online", "Online"),
                    ("running", "Running"),
                    ("stopping", "Stopping"),
                    ("error", "Error"),
                    ("offline", "Offline"),
                ],
                default="stopped",
                max_length=16,
            ),
        ),
    ]
