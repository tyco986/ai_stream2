from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("pipelines", "0001_initial"),
    ]

    operations = [
        migrations.AddField(
            model_name="pipeline",
            name="host_port",
            field=models.IntegerField(blank=True, null=True, unique=True),
        ),
    ]
