from django.contrib import admin

from .models import AnalyticsZone, Camera, CameraGroup


@admin.register(CameraGroup)
class CameraGroupAdmin(admin.ModelAdmin):
    list_display = ("name", "organization", "created_at")
    list_filter = ("organization",)


@admin.register(Camera)
class CameraAdmin(admin.ModelAdmin):
    list_display = ("name", "uid", "status", "organization", "group", "is_deleted")
    list_filter = ("status", "organization", "is_deleted")
    search_fields = ("name", "uid", "rtsp_url")


@admin.register(AnalyticsZone)
class AnalyticsZoneAdmin(admin.ModelAdmin):
    list_display = ("name", "camera", "zone_type", "is_enabled")
    list_filter = ("zone_type", "is_enabled")
