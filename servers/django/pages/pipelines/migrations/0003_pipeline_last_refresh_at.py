from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("pipelines", "0002_pipeline_host_port"),
    ]

    operations = [
        migrations.AddField(
            model_name="pipeline",
            name="last_refresh_at",
            field=models.DateTimeField(blank=True, null=True),
        ),
    ]
