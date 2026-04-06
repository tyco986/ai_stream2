from django.urls import include, path
from rest_framework.routers import DefaultRouter

from . import views

router = DefaultRouter()
router.register("detections", views.DetectionViewSet, basename="detection")

urlpatterns = [
    path("", include(router.urls)),
]
