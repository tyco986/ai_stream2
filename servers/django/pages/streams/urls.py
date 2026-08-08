from django.urls import path

from pages.streams.views import (
    BatchDeleteView,
    BatchDisableView,
    BatchEnableView,
    BatchProbeView,
    BatchRecordView,
    BatchUnrecordView,
    GroupCandidatesView,
    GroupCreateOrDetailView,
    GroupMembersView,
    GroupsListView,
    GroupsMapView,
    GroupsTreeView,
    StreamDetailView,
    StreamIdProbeView,
    StreamListCreateView,
    StreamLogsView,
    StreamProbeView,
    StreamPublishersView,
    StreamsMapView,
)

urlpatterns = [
    path("groups/tree", GroupsTreeView.as_view(), name="streams-groups-tree"),
    path("groups/map", GroupsMapView.as_view(), name="streams-groups-map"),
    path("groups/list", GroupsListView.as_view(), name="streams-groups-list"),
    path(
        "groups/<uuid:group_id>/members",
        GroupMembersView.as_view(),
        name="streams-group-members",
    ),
    path(
        "groups/<uuid:group_id>/candidates",
        GroupCandidatesView.as_view(),
        name="streams-group-candidates",
    ),
    path(
        "groups/<uuid:group_id>",
        GroupCreateOrDetailView.as_view(),
        name="streams-group-detail",
    ),
    path("map", StreamsMapView.as_view(), name="streams-map"),
    path("batch/delete", BatchDeleteView.as_view(), name="streams-batch-delete"),
    path("batch/enable", BatchEnableView.as_view(), name="streams-batch-enable"),
    path("batch/disable", BatchDisableView.as_view(), name="streams-batch-disable"),
    path("batch/record", BatchRecordView.as_view(), name="streams-batch-record"),
    path("batch/unrecord", BatchUnrecordView.as_view(), name="streams-batch-unrecord"),
    path("batch/probe", BatchProbeView.as_view(), name="streams-batch-probe"),
    path("probe", StreamProbeView.as_view(), name="streams-probe"),
    path("publishers", StreamPublishersView.as_view(), name="streams-publishers"),
    path("", StreamListCreateView.as_view(), name="streams-list"),
    path(
        "<uuid:stream_id>/probe",
        StreamIdProbeView.as_view(),
        name="streams-probe-id",
    ),
    path(
        "<uuid:stream_id>/logs",
        StreamLogsView.as_view(),
        name="streams-logs",
    ),
    path(
        "<uuid:stream_id>",
        StreamDetailView.as_view(),
        name="streams-detail",
    ),
]
