from django.contrib import admin
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin

from .models import Organization, User


@admin.register(Organization)
class OrganizationAdmin(admin.ModelAdmin):
    list_display = ("name", "created_at")
    search_fields = ("name",)


@admin.register(User)
class UserAdmin(BaseUserAdmin):
    list_display = ("username", "email", "organization", "role", "is_active")
    list_filter = ("role", "organization", "is_active")
    fieldsets = BaseUserAdmin.fieldsets + (
        ("扩展信息", {"fields": ("organization", "role")}),
    )
