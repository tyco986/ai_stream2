from django.db.models import Count, Sum
from django.db.models.functions import TruncHour
from django.utils.timezone import now, timedelta
from rest_framework.permissions import IsAuthenticated
from rest_framework.views import APIView

from apps.alerts.models import Alert
from apps.cameras.models import Camera
from apps.detections.models import Detection
from common.response import success_response


class OverviewView(APIView):
    permission_classes = [IsAuthenticated]

    def get(self, request):
        org = request.user.organization
        today_start = now().replace(hour=0, minute=0, second=0, microsecond=0)

        online_cameras = Camera.objects.filter(
            organization=org, is_deleted=False, status="online",
        ).count()
        total_cameras = Camera.objects.filter(
            organization=org, is_deleted=False,
        ).count()
        today_detections = Detection.objects.filter(
            camera__organization=org, detected_at__gte=today_start,
        ).count()
        pending_alerts = Alert.objects.filter(
            organization=org, status="pending",
        ).count()

        return success_response({
            "online_cameras": online_cameras,
            "total_cameras": total_cameras,
            "today_detections": today_detections,
            "pending_alerts": pending_alerts,
        })


class DetectionTrendView(APIView):
    permission_classes = [IsAuthenticated]

    def get(self, request):
        org = request.user.organization
        hours = int(request.query_params.get("hours", 24))
        since = now() - timedelta(hours=hours)

        trend = (
            Detection.objects.filter(
                camera__organization=org, detected_at__gte=since,
            )
            .annotate(hour=TruncHour("detected_at"))
            .values("hour")
            .annotate(count=Count("id"), total_objects=Sum("object_count"))
            .order_by("hour")
        )
        return success_response(list(trend))


class CameraStatusView(APIView):
    permission_classes = [IsAuthenticated]

    def get(self, request):
        org = request.user.organization
        status_counts = (
            Camera.objects.filter(organization=org, is_deleted=False)
            .values("status")
            .annotate(count=Count("id"))
        )
        return success_response(list(status_counts))
