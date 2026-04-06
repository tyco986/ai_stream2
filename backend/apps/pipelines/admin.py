from django.contrib import admin

from .models import AIModel, CameraModelBinding, PipelineProfile


@admin.register(AIModel)
class AIModelAdmin(admin.ModelAdmin):
    list_display = ("name", "model_type", "framework", "organization", "is_active", "version")
    list_filter = ("model_type", "framework", "is_active", "organization")


@admin.register(PipelineProfile)
class PipelineProfileAdmin(admin.ModelAdmin):
    list_display = ("name", "organization", "detector", "tracker", "analytics_enabled", "is_active")
    list_filter = ("is_active", "analytics_enabled", "organization")


@admin.register(CameraModelBinding)
class CameraModelBindingAdmin(admin.ModelAdmin):
    list_display = ("camera", "pipeline_profile", "is_enabled")
    list_filter = ("is_enabled",)
