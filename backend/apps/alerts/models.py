from django.db import models

from common.models import BaseModel


class AlertRule(BaseModel):
    class RuleType(models.TextChoices):
        OBJECT_COUNT = "object_count", "对象数量"
        OBJECT_TYPE = "object_type", "对象类型"
        ZONE_INTRUSION = "zone_intrusion", "区域入侵"
        LINE_CROSSING = "line_crossing", "越线检测"
        OVERCROWDING = "overcrowding", "拥挤检测"

    name = models.CharField("规则名称", max_length=200)
    organization = models.ForeignKey(
        "accounts.Organization",
        on_delete=models.CASCADE,
        related_name="alert_rules",
        verbose_name="所属组织",
    )
    rule_type = models.CharField(
        "规则类型",
        max_length=30,
        choices=RuleType.choices,
    )
    conditions = models.JSONField(
        "规则条件",
        help_text="Schema 因 rule_type 而异",
    )
    cameras = models.ManyToManyField(
        "cameras.Camera",
        related_name="alert_rules",
        verbose_name="关联摄像头",
        blank=True,
        help_text="空 = 全部摄像头",
    )
    is_enabled = models.BooleanField("是否启用", default=True)
    cooldown_seconds = models.IntegerField("冷却时间(秒)", default=60)
    notify_channels = models.JSONField(
        "通知渠道",
        default=list,
        help_text='["websocket", "email", "webhook"]',
    )

    class Meta(BaseModel.Meta):
        verbose_name = "报警规则"
        verbose_name_plural = "报警规则"

    def __str__(self):
        return f"{self.name} ({self.rule_type})"


class Alert(BaseModel):
    class Status(models.TextChoices):
        PENDING = "pending", "待处理"
        ACKNOWLEDGED = "acknowledged", "已确认"
        RESOLVED = "resolved", "已解决"

    rule = models.ForeignKey(
        AlertRule,
        on_delete=models.CASCADE,
        related_name="alerts",
        verbose_name="触发规则",
    )
    camera = models.ForeignKey(
        "cameras.Camera",
        on_delete=models.CASCADE,
        related_name="alerts",
        verbose_name="触发摄像头",
    )
    organization = models.ForeignKey(
        "accounts.Organization",
        on_delete=models.CASCADE,
        related_name="alerts",
        verbose_name="所属组织",
    )
    triggered_at = models.DateTimeField("触发时间", db_index=True)
    status = models.CharField(
        "状态",
        max_length=20,
        choices=Status.choices,
        default=Status.PENDING,
    )
    snapshot = models.JSONField("触发时检测数据快照", default=dict)
    acknowledged_by = models.ForeignKey(
        "accounts.User",
        on_delete=models.SET_NULL,
        related_name="acknowledged_alerts",
        verbose_name="确认人",
        null=True,
        blank=True,
    )
    acknowledged_at = models.DateTimeField("确认时间", null=True, blank=True)
    resolved_by = models.ForeignKey(
        "accounts.User",
        on_delete=models.SET_NULL,
        related_name="resolved_alerts",
        verbose_name="解决人",
        null=True,
        blank=True,
    )
    resolved_at = models.DateTimeField("解决时间", null=True, blank=True)

    class Meta(BaseModel.Meta):
        verbose_name = "报警记录"
        verbose_name_plural = "报警记录"

    def __str__(self):
        return f"Alert({self.id}) rule={self.rule.name} status={self.status}"
