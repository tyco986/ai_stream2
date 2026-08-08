from django.urls import path

from pages.pipelines.views.views_gie import (
    GieTemplateBatchDeleteView,
    GieTemplateDetailView,
    GieTemplateListCreateView,
)

urlpatterns = [
    path("batch/delete", GieTemplateBatchDeleteView.as_view(), name="gie-templates-batch-delete"),
    path("", GieTemplateListCreateView.as_view(), name="gie-templates-list"),
    path("<uuid:gie_id>", GieTemplateDetailView.as_view(), name="gie-templates-detail"),
]
