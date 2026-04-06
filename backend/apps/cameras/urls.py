from django.urls import include, path
from rest_framework.routers import DefaultRouter

from . import views

router = DefaultRouter()
router.register("cameras", views.CameraViewSet, basename="camera")
router.register("camera-groups", views.CameraGroupViewSet, basename="camera-group")

urlpatterns = [
    path("", include(router.urls)),
    path(
        "cameras/<uuid:camera_pk>/analytics-zones/",
        views.AnalyticsZoneViewSet.as_view({"get": "list", "post": "create"}),
        name="analytics-zone-list",
    ),
    path(
        "cameras/<uuid:camera_pk>/analytics-zones/<uuid:pk>/",
        views.AnalyticsZoneViewSet.as_view({"get": "retrieve", "patch": "partial_update", "delete": "destroy"}),
        name="analytics-zone-detail",
    ),
]
