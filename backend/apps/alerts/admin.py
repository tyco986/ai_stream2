from django.contrib import admin

from .models import Alert, AlertRule


@admin.register(AlertRule)
class AlertRuleAdmin(admin.ModelAdmin):
    list_display = ("name", "rule_type", "organization", "is_enabled", "cooldown_seconds")
    list_filter = ("rule_type", "is_enabled", "organization")


@admin.register(Alert)
class AlertAdmin(admin.ModelAdmin):
    list_display = ("id", "rule", "camera", "status", "triggered_at")
    list_filter = ("status", "organization")
    date_hierarchy = "triggered_at"
