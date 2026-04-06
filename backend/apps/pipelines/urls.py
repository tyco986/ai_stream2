from django.urls import include, path
from rest_framework.routers import DefaultRouter

from . import views

router = DefaultRouter()
router.register("ai-models", views.AIModelViewSet, basename="ai-model")
router.register("pipeline-profiles", views.PipelineProfileViewSet, basename="pipeline-profile")

urlpatterns = [
    path("", include(router.urls)),
]
