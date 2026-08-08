from django.apps import AppConfig


class ModelsConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "pages.models_page"
    label = "models"

    def ready(self):
        from pages.models_page.model_lookup import register_model_built_resolver
        from pages.models_page.site_config import register_models_site_config

        register_models_site_config()
        register_model_built_resolver()
