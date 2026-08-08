from django.apps import AppConfig


class PipelinesConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "pages.pipelines"
    label = "pipelines"

    def ready(self):
        from pages.pipelines.site_config import register_pipelines_site_config

        register_pipelines_site_config()
