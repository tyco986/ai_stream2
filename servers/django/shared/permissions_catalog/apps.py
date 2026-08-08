from django.apps import AppConfig


class PermissionsCatalogConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "shared.permissions_catalog"
    label = "permissions_catalog"

    def ready(self):
        from shared.permissions_catalog import signals  # noqa: F401
