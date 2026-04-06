from django.db.models import Count, Sum
from django.db.models.functions import TruncHour
from django.utils.dateparse import parse_datetime
from rest_framework import viewsets
from rest_framework.decorators import action

from common.pagination import DetectionCursorPagination
from common.permissions import IsViewer
from common.response import success_response

from .models import Detection
from .serializers import DetectionSerializer


class DetectionViewSet(viewsets.ReadOnlyModelViewSet):
    serializer_class = DetectionSerializer
    pagination_class = DetectionCursorPagination
    permission_classes = [IsViewer]

    def get_queryset(self):
        qs = Detection.objects.filter(
            camera__organization=self.request.user.organization,
        ).select_related("camera")

        camera_id = self.request.query_params.get("camera_id")
        if camera_id:
            qs = qs.filter(camera_id=camera_id)

        start_time = self.request.query_params.get("start_time")
        if start_time:
            parsed = parse_datetime(start_time)
            if parsed:
                qs = qs.filter(detected_at__gte=parsed)

        end_time = self.request.query_params.get("end_time")
        if end_time:
            parsed = parse_datetime(end_time)
            if parsed:
                qs = qs.filter(detected_at__lte=parsed)

        object_type = self.request.query_params.get("object_type")
        if object_type:
            qs = qs.filter(detected_objects__contains=[{"type": object_type}])

        return qs

    @action(detail=False, methods=["get"])
    def stats(self, request):
        qs = self.get_queryset()
        hourly = (
            qs.annotate(hour=TruncHour("detected_at"))
            .values("hour")
            .annotate(count=Count("id"), total_objects=Sum("object_count"))
            .order_by("hour")
        )
        return success_response(list(hourly[:168]))
