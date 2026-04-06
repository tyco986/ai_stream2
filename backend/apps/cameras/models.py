from django.db import models

from common.models import BaseModel


class ActiveManager(models.Manager):
    def get_queryset(self):
        return super().get_queryset().filter(is_deleted=False)


class CameraGroup(BaseModel):
    name = models.CharField("分组名称", max_length=200)
    organization = models.ForeignKey(
        "accounts.Organization",
        on_delete=models.CASCADE,
        related_name="camera_groups",
        verbose_name="所属组织",
    )
    description = models.TextField("描述", blank=True, default="")

    class Meta(BaseModel.Meta):
        verbose_name = "摄像头分组"
        verbose_name_plural = "摄像头分组"

    def __str__(self):
        return self.name


class Camera(BaseModel):
    class Status(models.TextChoices):
        OFFLINE = "offline", "离线"
        CONNECTING = "connecting", "连接中"
        ONLINE = "online", "在线"
        ERROR = "error", "异常"

    uid = models.CharField(
        "DeepStream Camera ID",
        max_length=100,
        unique=True,
        help_text="对应 DeepStream camera_id / Kafka sensorId",
    )
    name = models.CharField("摄像头名称", max_length=200)
    rtsp_url = models.CharField("RTSP 地址", max_length=500)
    organization = models.ForeignKey(
        "accounts.Organization",
        on_delete=models.CASCADE,
        related_name="cameras",
        verbose_name="所属组织",
    )
    group = models.ForeignKey(
        CameraGroup,
        on_delete=models.SET_NULL,
        related_name="cameras",
        verbose_name="所属分组",
        null=True,
        blank=True,
    )
    status = models.CharField(
        "状态",
        max_length=20,
        choices=Status.choices,
        default=Status.OFFLINE,
    )
    is_deleted = models.BooleanField("已删除", default=False)
    config = models.JSONField("扩展配置", default=dict, blank=True)

    objects = ActiveManager()
    all_objects = models.Manager()

    class Meta(BaseModel.Meta):
        verbose_name = "摄像头"
        verbose_name_plural = "摄像头"

    def __str__(self):
        return f"{self.name} ({self.uid})"


class AnalyticsZone(BaseModel):
    class ZoneType(models.TextChoices):
        ROI = "roi", "ROI 区域"
        LINE_CROSSING = "line_crossing", "越线检测"
        OVERCROWDING = "overcrowding", "拥挤检测"
        DIRECTION = "direction", "方向检测"

    camera = models.ForeignKey(
        Camera,
        on_delete=models.CASCADE,
        related_name="analytics_zones",
        verbose_name="关联摄像头",
    )
    name = models.CharField("区域名称", max_length=200)
    zone_type = models.CharField(
        "区域类型",
        max_length=20,
        choices=ZoneType.choices,
    )
    coordinates = models.JSONField(
        "坐标点列表",
        help_text="[[x1,y1], [x2,y2], ...] 基于 1920x1080 分辨率",
    )
    config = models.JSONField("类型特定配置", default=dict, blank=True)
    is_enabled = models.BooleanField("是否启用", default=True)

    class Meta(BaseModel.Meta):
        verbose_name = "分析区域"
        verbose_name_plural = "分析区域"

    def __str__(self):
        return f"{self.camera.uid}:{self.name} ({self.zone_type})"
