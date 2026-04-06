from django.urls import path

from . import views

urlpatterns = [
    path("dashboard/overview/", views.OverviewView.as_view(), name="dashboard-overview"),
    path("dashboard/detection-trend/", views.DetectionTrendView.as_view(), name="dashboard-detection-trend"),
    path("dashboard/camera-status/", views.CameraStatusView.as_view(), name="dashboard-camera-status"),
]
