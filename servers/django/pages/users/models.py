import uuid

from django.conf import settings
from django.contrib.auth.models import AbstractUser, Group
from django.db import models
from django.utils import timezone


class User(AbstractUser):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    must_change_password = models.BooleanField(default=False)

    class Meta:
        db_table = "users_user"


class PermissionHost(models.Model):
    """Anchor ContentType for product PermissionCatalog codenames."""

    class Meta:
        managed = False
        default_permissions = ()
        permissions = (
            ("export_site_config", "Can export site config"),
            ("import_site_config", "Can import site config"),
        )


class GroupUuid(models.Model):
    """UUID facade over Django auth.Group (API exposes UUID only)."""

    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    group = models.OneToOneField(
        Group,
        on_delete=models.CASCADE,
        related_name="uuid_map",
    )

    class Meta:
        db_table = "users_groupuuid"


class UserAuditLog(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    user = models.ForeignKey(
        settings.AUTH_USER_MODEL,
        on_delete=models.CASCADE,
        related_name="audit_logs",
    )
    at = models.DateTimeField(default=timezone.now, db_index=True)
    label = models.CharField(max_length=64)
    detail = models.TextField()

    class Meta:
        db_table = "users_userauditlog"
        ordering = ["-at"]
