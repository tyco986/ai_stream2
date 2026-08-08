from django.urls import path

from pages.models_page.views import (
    ModelBatchDeleteView,
    ModelBuildStatusView,
    ModelBuildView,
    ModelDetailView,
    ModelListCreateView,
    ModelLogsView,
    ModelMapView,
    ModelTypesView,
)

urlpatterns = [
    path("map", ModelMapView.as_view(), name="models-map"),
    path("types", ModelTypesView.as_view(), name="models-types"),
    path("batch/delete", ModelBatchDeleteView.as_view(), name="models-batch-delete"),
    path(
        "<uuid:model_id>/build/status",
        ModelBuildStatusView.as_view(),
        name="models-build-status",
    ),
    path(
        "<uuid:model_id>/build",
        ModelBuildView.as_view(),
        name="models-build",
    ),
    path(
        "<uuid:model_id>/logs",
        ModelLogsView.as_view(),
        name="models-logs",
    ),
    path(
        "<uuid:model_id>",
        ModelDetailView.as_view(),
        name="models-detail",
    ),
    path("", ModelListCreateView.as_view(), name="models-list"),
]
