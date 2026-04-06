from datetime import timedelta

import structlog
from asgiref.sync import async_to_sync
from celery import shared_task
from django.conf import settings
from django.db import connection
from django.utils.timezone import now

logger = structlog.get_logger(__name__)


@shared_task(bind=True, max_retries=3, retry_backoff=True)
def cleanup_old_detections(self):
    """DROP 超过 DETECTION_RETENTION_MONTHS 的 Detection 分区。"""
    retention = settings.DETECTION_RETENTION_MONTHS
    cutoff = now() - timedelta(days=retention * 30)
    cutoff_str = cutoff.strftime("%Y_%m")

    with connection.cursor() as cursor:
        cursor.execute(
            "SELECT tablename FROM pg_tables "
            "WHERE tablename LIKE 'detections_detection_%%' ORDER BY tablename"
        )
        partitions = [row[0] for row in cursor.fetchall()]

    dropped = []
    for partition_name in partitions:
        parts = partition_name.rsplit("_", 2)
        if len(parts) >= 3:
            partition_date = f"{parts[-2]}_{parts[-1]}"
            if partition_date < cutoff_str:
                with connection.cursor() as cursor:
                    cursor.execute(f"DROP TABLE IF EXISTS {partition_name}")
                dropped.append(partition_name)

    logger.info("Detection partition cleanup done",
                retention_months=retention, dropped=dropped)


@shared_task(bind=True, max_retries=3, retry_backoff=True)
def create_next_partition(self):
    """创建下月 Detection 分区。"""
    next_month = now().replace(day=1) + timedelta(days=32)
    next_month = next_month.replace(day=1)
    month_after = next_month + timedelta(days=32)
    month_after = month_after.replace(day=1)

    partition_name = f"detections_detection_{next_month.strftime('%Y_%m')}"
    start_date = next_month.strftime("%Y-%m-01")
    end_date = month_after.strftime("%Y-%m-01")

    sql = (
        f"CREATE TABLE IF NOT EXISTS {partition_name} "
        f"PARTITION OF detections_detection "
        f"FOR VALUES FROM ('{start_date}') TO ('{end_date}')"
    )

    with connection.cursor() as cursor:
        cursor.execute(sql)

    logger.info("Detection partition created", partition=partition_name)


@shared_task(bind=True, max_retries=3, retry_backoff=True)
def sync_camera_status(self):
    """从 DeepStream 查询实际流状态，同步到数据库。"""
    from apps.cameras.models import Camera
    from services.deepstream_client import deepstream_client

    try:
        resp = async_to_sync(deepstream_client.get_stream_info)()
        data = resp.json()
    except Exception:
        logger.warning("Failed to query DeepStream stream info", exc_info=True)
        return

    stream_info = data.get("stream-info", {}).get("stream-info", [])
    active_uids = set()
    for entry in stream_info:
        uid = entry.get("camera_id") or entry.get("cameraId") or entry.get("sensor_id")
        if uid:
            active_uids.add(uid)

    cameras = Camera.objects.filter(is_deleted=False, status__in=["online", "connecting", "error"])
    updated = 0
    for camera in cameras:
        if camera.uid not in active_uids and camera.status != "offline":
            camera.status = Camera.Status.ERROR
            camera.save(update_fields=["status", "updated_at"])
            updated += 1

    offline_cameras = Camera.objects.filter(is_deleted=False, status="offline")
    for camera in offline_cameras:
        if camera.uid in active_uids:
            camera.status = Camera.Status.ONLINE
            camera.save(update_fields=["status", "updated_at"])
            updated += 1

    if updated:
        logger.info("Camera status synced", updated=updated, active_streams=len(active_uids))


@shared_task(bind=True, max_retries=3, retry_backoff=True)
def cleanup_dead_letters(self):
    """删除超过 DEAD_LETTER_RETENTION_DAYS 的 KafkaDeadLetter 记录。"""
    from apps.detections.models import KafkaDeadLetter

    retention_days = settings.DEAD_LETTER_RETENTION_DAYS
    cutoff = now() - timedelta(days=retention_days)
    deleted_count, _ = KafkaDeadLetter.objects.filter(created_at__lt=cutoff).delete()
    logger.info("Dead letter cleanup done",
                retention_days=retention_days, deleted=deleted_count)
