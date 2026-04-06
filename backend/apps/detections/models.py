from django.db import models

from common.models import BaseModel


class Detection(models.Model):
    """检测记录 — 高频写入，使用 BigAutoField + 声明式分区。

    与其他模型不同，此模型使用 BigAutoField 而非 UUID 主键，
    因为日均 140 万行写入下 UUID 随机值导致 B-tree 页分裂。
    """

    id = models.BigAutoField(primary_key=True)
    camera = models.ForeignKey(
        "cameras.Camera",
        on_delete=models.CASCADE,
        related_name="detections",
        verbose_name="来源摄像头",
    )
    detected_at = models.DateTimeField("检测时间", db_index=True)
    ingested_at = models.DateTimeField("入库时间", auto_now_add=True)
    frame_number = models.BigIntegerField("帧号", null=True, blank=True)
    object_count = models.IntegerField("检测对象数", default=0)
    detected_objects = models.JSONField(
        "检测对象",
        default=list,
        db_column="objects",
        help_text="[{type, confidence, bbox, object_id, classifier?, analytics?}]",
    )
    analytics = models.JSONField(
        "帧级分析结果",
        null=True,
        blank=True,
        help_text="越线计数、拥挤状态等 nvdsanalytics 帧级结果",
    )

    class Meta:
        ordering = ["-detected_at"]
        indexes = [
            models.Index(fields=["camera", "-detected_at"]),
            models.Index(fields=["-detected_at"]),
        ]
        verbose_name = "检测记录"
        verbose_name_plural = "检测记录"

    def __str__(self):
        return f"Detection({self.id}) camera={self.camera_id} at={self.detected_at}"


class KafkaDeadLetter(BaseModel):
    """Kafka 消费失败的死信记录。"""

    topic = models.CharField("Kafka Topic", max_length=200)
    partition_num = models.IntegerField("分区号")
    offset = models.BigIntegerField("Offset")
    raw_message = models.TextField("原始消息内容")
    error_message = models.TextField("错误信息")

    class Meta(BaseModel.Meta):
        verbose_name = "Kafka 死信"
        verbose_name_plural = "Kafka 死信"

    def __str__(self):
        return f"DeadLetter({self.topic}:{self.partition_num}:{self.offset})"
