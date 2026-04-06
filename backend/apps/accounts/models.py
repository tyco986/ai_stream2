import uuid

from django.contrib.auth.models import AbstractUser
from django.db import models

from common.models import BaseModel


class Organization(BaseModel):
    name = models.CharField("组织名称", max_length=200)

    class Meta(BaseModel.Meta):
        verbose_name = "组织"
        verbose_name_plural = "组织"

    def __str__(self):
        return self.name


class User(AbstractUser):
    class Role(models.TextChoices):
        ADMIN = "admin", "管理员"
        OPERATOR = "operator", "操作员"
        VIEWER = "viewer", "观察者"

    id = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    organization = models.ForeignKey(
        Organization,
        on_delete=models.CASCADE,
        related_name="users",
        verbose_name="所属组织",
        null=True,
        blank=True,
    )
    role = models.CharField(
        "角色",
        max_length=20,
        choices=Role.choices,
        default=Role.VIEWER,
    )

    class Meta:
        verbose_name = "用户"
        verbose_name_plural = "用户"

    def __str__(self):
        return self.username
