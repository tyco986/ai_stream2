from django.urls import path

from pages.preview.views import (
    ActiveLayoutView,
    LayoutBatchDeleteView,
    LayoutDetailView,
    LayoutListCreateView,
    LayoutShotView,
    LayoutsMapView,
)

urlpatterns = [
    path("active_layout", ActiveLayoutView.as_view(), name="preview-active-layout"),
    path("layouts/map", LayoutsMapView.as_view(), name="preview-layouts-map"),
    path(
        "layouts/batch/delete",
        LayoutBatchDeleteView.as_view(),
        name="preview-layouts-batch-delete",
    ),
    path(
        "layouts/<uuid:preset_id>/shot",
        LayoutShotView.as_view(),
        name="preview-layout-shot",
    ),
    path(
        "layouts/<uuid:preset_id>",
        LayoutDetailView.as_view(),
        name="preview-layout-detail",
    ),
    path("layouts", LayoutListCreateView.as_view(), name="preview-layouts"),
]
