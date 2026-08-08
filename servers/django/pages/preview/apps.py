from django.apps import AppConfig


class PreviewConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "pages.preview"
    label = "previews"

    def ready(self):
        from pages.preview.site_config import register_preview_site_config

        register_preview_site_config()
