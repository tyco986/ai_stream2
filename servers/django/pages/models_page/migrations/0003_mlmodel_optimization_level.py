from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("models", "0002_mlmodel_family"),
    ]

    operations = [
        migrations.AddField(
            model_name="mlmodel",
            name="optimization_level",
            field=models.PositiveSmallIntegerField(default=3),
        ),
    ]
