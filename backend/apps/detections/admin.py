from django.contrib import admin

from .models import Detection, KafkaDeadLetter


@admin.register(Detection)
class DetectionAdmin(admin.ModelAdmin):
    list_display = ("id", "camera", "detected_at", "object_count")
    list_filter = ("camera",)
    date_hierarchy = "detected_at"
    readonly_fields = ("ingested_at",)


@admin.register(KafkaDeadLetter)
class KafkaDeadLetterAdmin(admin.ModelAdmin):
    list_display = ("topic", "partition_num", "offset", "created_at")
    list_filter = ("topic",)
    readonly_fields = ("raw_message", "error_message")
