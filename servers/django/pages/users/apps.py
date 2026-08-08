from django.apps import AppConfig


class UsersConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "pages.users"
    label = "users"

    def ready(self):
        from pages.users.services import UserAuditService
        from pages.users.site_config import register_users_site_config
        from shared.audit import AuditService

        AuditService().register_sink(UserAuditService().record_sink)
        register_users_site_config()
