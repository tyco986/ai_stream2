from django.contrib.auth.models import Permission
from django.contrib.contenttypes.models import ContentType
from django.db.models.signals import post_migrate
from django.dispatch import receiver

# Codenames stored on users.PermissionHost → has_perm("users.<codename>").
# Skip User defaults and perms owned by page apps (streams/previews/recordings/models/pipelines).
CATALOG_PERMISSIONS = [
    ("view_group", "Can view group"),
    ("add_group", "Can add group"),
    ("change_group", "Can change group"),
    ("delete_group", "Can delete group"),
    ("export_site_config", "Can export site config"),
    ("import_site_config", "Can import site config"),
    ("view_server", "Can view server"),
    ("change_server", "Can change server"),
    ("view_event", "Can view event"),
    ("change_event", "Can change event"),
]


@receiver(post_migrate)
def ensure_catalog_permissions(sender, **kwargs):
    from pages.users.models import PermissionHost

    content_type = ContentType.objects.get_for_model(PermissionHost)
    for codename, name in CATALOG_PERMISSIONS:
        Permission.objects.get_or_create(
            content_type=content_type,
            codename=codename,
            defaults={"name": name},
        )
