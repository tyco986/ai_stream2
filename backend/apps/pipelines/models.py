from django.db import models

from common.models import BaseModel


class AIModel(BaseModel):
    class ModelType(models.TextChoices):
        DETECTOR = "detector", "检测器"
        TRACKER = "tracker", "跟踪器"

    class Framework(models.TextChoices):
        ONNX = "onnx", "ONNX"
        ENGINE = "engine", "TensorRT Engine"
        CUSTOM = "custom", "自定义"

    name = models.CharField("模型名称", max_length=200)
    organization = models.ForeignKey(
        "accounts.Organization",
        on_delete=models.CASCADE,
        related_name="ai_models",
        verbose_name="所属组织",
    )
    model_type = models.CharField(
        "模型类型",
        max_length=20,
        choices=ModelType.choices,
    )
    framework = models.CharField(
        "模型框架",
        max_length=20,
        choices=Framework.choices,
        default=Framework.ONNX,
    )
    model_file = models.CharField("模型文件路径", max_length=500)
    label_file = models.CharField("标签文件路径", max_length=500, blank=True, default="")
    config = models.JSONField(
        "模型配置",
        default=dict,
        help_text="因 model_type 而异的配置参数",
    )
    version = models.CharField("版本号", max_length=50, default="1.0")
    description = models.TextField("描述", blank=True, default="")
    is_active = models.BooleanField("是否可用", default=True)

    class Meta(BaseModel.Meta):
        verbose_name = "AI 模型"
        verbose_name_plural = "AI 模型"
        unique_together = [("organization", "name")]

    def __str__(self):
        return f"{self.name} ({self.model_type})"


class PipelineProfile(BaseModel):
    name = models.CharField("管道名称", max_length=200)
    organization = models.ForeignKey(
        "accounts.Organization",
        on_delete=models.CASCADE,
        related_name="pipeline_profiles",
        verbose_name="所属组织",
    )
    description = models.TextField("描述", blank=True, default="")
    detector = models.ForeignKey(
        AIModel,
        on_delete=models.PROTECT,
        related_name="pipeline_as_detector",
        verbose_name="主检测模型",
        limit_choices_to={"model_type": "detector"},
    )
    tracker = models.ForeignKey(
        AIModel,
        on_delete=models.SET_NULL,
        related_name="pipeline_as_tracker",
        verbose_name="跟踪配置",
        limit_choices_to={"model_type": "tracker"},
        null=True,
        blank=True,
    )
    analytics_enabled = models.BooleanField("启用 nvdsanalytics", default=True)
    analytics_config_stale = models.BooleanField(
        "分析配置需要重新部署",
        default=False,
        help_text="摄像头增删后自动标记，提示用户重新部署",
    )
    is_active = models.BooleanField("是否启用", default=True)

    class Meta(BaseModel.Meta):
        verbose_name = "管道配置"
        verbose_name_plural = "管道配置"

    def __str__(self):
        return self.name


class CameraModelBinding(BaseModel):
    camera = models.OneToOneField(
        "cameras.Camera",
        on_delete=models.CASCADE,
        related_name="model_binding",
        verbose_name="关联摄像头",
    )
    pipeline_profile = models.ForeignKey(
        PipelineProfile,
        on_delete=models.CASCADE,
        related_name="camera_bindings",
        verbose_name="管道配置",
    )
    is_enabled = models.BooleanField("是否启用", default=True)

    class Meta(BaseModel.Meta):
        verbose_name = "摄像头模型绑定"
        verbose_name_plural = "摄像头模型绑定"

    def __str__(self):
        return f"{self.camera.uid} -> {self.pipeline_profile.name}"
