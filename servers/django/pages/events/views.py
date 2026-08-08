from django.http import FileResponse, HttpResponse
from rest_framework.views import APIView

from pages.events.permissions import HasChangeEvent, HasViewEvent
from pages.events.serializers import (
    AckActionSerializer,
    BatchAckSerializer,
    CollectSerializer,
)
from pages.events.services import (
    EventAckService,
    EventExportService,
    EventQueryService,
)
from shared.http.exceptions import AppError
from shared.http.response import api_success
from shared.pagination import PaginationService


class EventOptionsView(APIView):
    permission_classes = [HasViewEvent]

    def get(self, request):
        pipeline_id = request.query_params.get("pipeline_id") or None
        data = EventQueryService().options_events(pipeline_id=pipeline_id)
        return api_success(data)


class EventCalendarView(APIView):
    permission_classes = [HasViewEvent]

    def get(self, request):
        queries = EventQueryService()
        year = PaginationService().parse_int(request.query_params.get("year"), 0, "year")
        month = PaginationService().parse_int(
            request.query_params.get("month"), 0, "month"
        )
        if year < 1 or month < 1 or month > 12:
            raise AppError("Invalid year or month", status_code=400)
        filters = queries.parse_filters(request.query_params)
        data = queries.calendar(year, month, filters)
        return api_success(data)


class EventListView(APIView):
    permission_classes = [HasViewEvent]

    def get(self, request):
        queries = EventQueryService()
        filters = queries.parse_filters(request.query_params)
        page_info = PaginationService().parse(request.query_params)
        data = queries.list_events(
            filters,
            page=page_info["page"],
            page_size=page_info["page_size"],
        )
        return api_success(data)


class EventDetailView(APIView):
    permission_classes = [HasViewEvent]

    def get(self, request, event_id):
        queries = EventQueryService()
        filters = queries.parse_filters(request.query_params)
        data = queries.get_detail(event_id, filters)
        return api_success(data)


class EventAckView(APIView):
    permission_classes = [HasChangeEvent]

    def post(self, request, event_id):
        serializer = AckActionSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = EventAckService().ack_one(
            event_id, serializer.validated_data["action"]
        )
        return api_success(data)


class EventBatchAckView(APIView):
    permission_classes = [HasChangeEvent]

    def post(self, request):
        serializer = BatchAckSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = EventAckService().ack_batch(
            serializer.validated_data["event_ids"],
            serializer.validated_data["action"],
        )
        return api_success(data)


class EventExportView(APIView):
    permission_classes = [HasViewEvent]

    def post(self, request):
        queries = EventQueryService()
        filters = queries.parse_filters(request.query_params)
        buffer = EventExportService().export_zip(filters)
        response = HttpResponse(buffer.getvalue(), content_type="application/zip")
        response["Content-Disposition"] = 'attachment; filename="events-export.zip"'
        return response


class EventCollectView(APIView):
    permission_classes = [HasChangeEvent]

    def post(self, request):
        serializer = CollectSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        queries = EventQueryService()
        filters = queries.parse_filters(request.query_params)
        buffer = EventExportService().collect_zip(
            filters,
            serializer.validated_data["passphrase"],
        )
        response = HttpResponse(buffer.getvalue(), content_type="application/zip")
        response["Content-Disposition"] = 'attachment; filename="events-collect.zip"'
        return response


class EventMediaView(APIView):
    permission_classes = [HasViewEvent]

    def get(self, request, rel):
        path = EventQueryService().resolve_media_path(rel)
        content_type = "image/jpeg"
        if path.suffix.lower() == ".png":
            content_type = "image/png"
        return FileResponse(path.open("rb"), content_type=content_type)
