import re
import uuid

from django.conf import settings
from django.db import models

VERSION_PATTERN = re.compile(r"^\d+(\.\d+)?$")


class PageSettings(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    user = models.OneToOneField(
        settings.AUTH_USER_MODEL,
        on_delete=models.CASCADE,
        related_name="shell_page_settings",
    )
    mode = models.CharField(max_length=16)

    class Meta:
        db_table = "shell_pagesettings"


class SiteConfigVersion(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    version = models.CharField(max_length=64, unique=True)
    description = models.TextField(null=True, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)
    is_current = models.BooleanField(default=False)
    payload_path = models.TextField()

    class Meta:
        db_table = "shell_siteconfigversion"
        ordering = ["-created_at"]


class UsedAuthTicket(models.Model):
    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    jti = models.CharField(max_length=128, unique=True)
    used_at = models.DateTimeField(auto_now_add=True)
    action = models.CharField(max_length=16)

    class Meta:
        db_table = "shell_usedauthticket"
