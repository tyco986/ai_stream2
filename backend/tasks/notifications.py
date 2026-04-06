import structlog
from celery import shared_task

from services.notification import notification_service

logger = structlog.get_logger(__name__)


@shared_task(
    bind=True,
    max_retries=3,
    default_retry_delay=60,
    retry_backoff=True,
    retry_jitter=True,
)
def send_alert_notification(self, alert_id):
    """发送报警通知（邮件/Webhook）。"""
    from apps.alerts.models import Alert

    try:
        alert = Alert.objects.select_related(
            "rule", "camera", "organization",
        ).get(id=alert_id)
        notification_service.send_alert_notification(alert)
        logger.info("Alert notification sent", alert_id=str(alert_id))
    except Alert.DoesNotExist:
        logger.warning("Alert not found, skipping notification", alert_id=str(alert_id))
    except Exception as exc:
        logger.exception("Alert notification failed, retrying", alert_id=str(alert_id))
        raise self.retry(exc=exc)
