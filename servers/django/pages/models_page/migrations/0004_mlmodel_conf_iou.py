from django.db import migrations, models


class Migration(migrations.Migration):
    dependencies = [
        ("models", "0003_mlmodel_optimization_level"),
    ]

    operations = [
        migrations.AddField(
            model_name="mlmodel",
            name="conf",
            field=models.FloatField(default=0.25),
        ),
        migrations.AddField(
            model_name="mlmodel",
            name="iou",
            field=models.FloatField(default=0.45),
        ),
    ]
