import structlog
from django.conf import settings
from django.core.mail import send_mail

logger = structlog.get_logger(__name__)


class NotificationService:
    """通知服务：邮件和 Webhook 分发（由 Celery 任务调用）。"""

    def send_alert_notification(self, alert):
        channels = alert.rule.notify_channels or []
        for channel in channels:
            if channel == "email":
                self._send_email(alert)
            elif channel == "webhook":
                self._send_webhook(alert)

    def _send_email(self, alert):
        try:
            subject = f"[AI Stream] 报警: {alert.rule.name}"
            body = (
                f"摄像头: {alert.camera.name} ({alert.camera.uid})\n"
                f"规则: {alert.rule.name}\n"
                f"触发时间: {alert.triggered_at}\n"
                f"状态: {alert.status}\n"
            )
            send_mail(
                subject,
                body,
                settings.DEFAULT_FROM_EMAIL if hasattr(settings, "DEFAULT_FROM_EMAIL") else "noreply@ai-stream.local",
                self._get_recipients(alert),
                fail_silently=False,
            )
            logger.info("Email notification sent", alert_id=str(alert.id))
        except Exception:
            logger.exception("Failed to send email notification", alert_id=str(alert.id))
            raise

    def _send_webhook(self, alert):
        import httpx

        webhook_url = getattr(settings, "ALERT_WEBHOOK_URL", None)
        if not webhook_url:
            logger.warning("No webhook URL configured, skipping")
            return
        try:
            payload = {
                "alert_id": str(alert.id),
                "rule_name": alert.rule.name,
                "camera_uid": alert.camera.uid,
                "triggered_at": alert.triggered_at.isoformat(),
                "status": alert.status,
            }
            with httpx.Client(timeout=10.0) as client:
                resp = client.post(webhook_url, json=payload)
                resp.raise_for_status()
            logger.info("Webhook notification sent", alert_id=str(alert.id))
        except Exception:
            logger.exception("Failed to send webhook notification", alert_id=str(alert.id))
            raise

    def _get_recipients(self, alert):
        """获取该组织 admin 角色用户的邮箱列表。"""
        from apps.accounts.models import User  # noqa: E402 — 解决循环依赖
        return list(
            User.objects.filter(
                organization=alert.organization,
                role="admin",
                is_active=True,
            ).exclude(email="").values_list("email", flat=True)
        )


notification_service = NotificationService()
