from django.urls import path

from pages.events.views import (
    EventAckView,
    EventBatchAckView,
    EventCalendarView,
    EventCollectView,
    EventDetailView,
    EventExportView,
    EventListView,
    EventOptionsView,
)

urlpatterns = [
    path("options/events", EventOptionsView.as_view(), name="events-options"),
    path("calendar", EventCalendarView.as_view(), name="events-calendar"),
    path("batch/ack", EventBatchAckView.as_view(), name="events-batch-ack"),
    path("export", EventExportView.as_view(), name="events-export"),
    path("collect", EventCollectView.as_view(), name="events-collect"),
    path("", EventListView.as_view(), name="events-list"),
    path("<uuid:event_id>/ack", EventAckView.as_view(), name="events-ack"),
    path("<uuid:event_id>", EventDetailView.as_view(), name="events-detail"),
]
