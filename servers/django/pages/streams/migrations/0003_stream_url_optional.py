from django.db import migrations, models


def empty_url_to_null(apps, schema_editor):
    Stream = apps.get_model("streams", "Stream")
    Stream.objects.filter(url="").update(url=None)


class Migration(migrations.Migration):

    dependencies = [
        ("streams", "0002_stream_url_unique"),
    ]

    operations = [
        migrations.RunPython(empty_url_to_null, migrations.RunPython.noop),
        migrations.AlterField(
            model_name="stream",
            name="url",
            field=models.TextField(blank=True, null=True, unique=True),
        ),
    ]
