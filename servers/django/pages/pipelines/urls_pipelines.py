from django.urls import path

from pages.pipelines.views.views_pipelines import (
    PipelineBatchDeleteView,
    PipelineDetailView,
    PipelineListCreateView,
    PipelineLogsView,
    PipelineMapView,
    PipelineSchemaView,
    PipelineStartView,
    PipelineStatusView,
    PipelineStopView,
    PipelineTypesView,
)

urlpatterns = [
    path("types", PipelineTypesView.as_view(), name="pipelines-types"),
    path("schema", PipelineSchemaView.as_view(), name="pipelines-schema"),
    path("map", PipelineMapView.as_view(), name="pipelines-map"),
    path("batch/delete", PipelineBatchDeleteView.as_view(), name="pipelines-batch-delete"),
    path("", PipelineListCreateView.as_view(), name="pipelines-list"),
    path(
        "<uuid:pipeline_id>/start",
        PipelineStartView.as_view(),
        name="pipelines-start",
    ),
    path(
        "<uuid:pipeline_id>/stop",
        PipelineStopView.as_view(),
        name="pipelines-stop",
    ),
    path(
        "<uuid:pipeline_id>/status",
        PipelineStatusView.as_view(),
        name="pipelines-status",
    ),
    path(
        "<uuid:pipeline_id>/logs",
        PipelineLogsView.as_view(),
        name="pipelines-logs",
    ),
    path(
        "<uuid:pipeline_id>",
        PipelineDetailView.as_view(),
        name="pipelines-detail",
    ),
]
