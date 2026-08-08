from django.urls import path

from pages.pipelines.views.views_analyzer import (
    AnalyzerSourceFileView,
    AnalyzerSourceStreamView,
    AnalyzerTemplateBatchDeleteView,
    AnalyzerTemplateDetailView,
    AnalyzerTemplateListCreateView,
)

urlpatterns = [
    path(
        "batch/delete",
        AnalyzerTemplateBatchDeleteView.as_view(),
        name="analyzer-templates-batch-delete",
    ),
    path("", AnalyzerTemplateListCreateView.as_view(), name="analyzer-templates-list"),
    path(
        "<uuid:analyzer_template_id>/source/file",
        AnalyzerSourceFileView.as_view(),
        name="analyzer-templates-source-file",
    ),
    path(
        "<uuid:analyzer_template_id>/source/stream",
        AnalyzerSourceStreamView.as_view(),
        name="analyzer-templates-source-stream",
    ),
    path(
        "<uuid:analyzer_template_id>",
        AnalyzerTemplateDetailView.as_view(),
        name="analyzer-templates-detail",
    ),
]
