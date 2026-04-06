from django.urls import path

from . import health_views

urlpatterns = [
    path("live/", health_views.LivenessView.as_view(), name="health-live"),
    path("ready/", health_views.ReadinessView.as_view(), name="health-ready"),
]
