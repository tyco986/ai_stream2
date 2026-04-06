from django.urls import path

from . import deepstream_views

urlpatterns = [
    path("health/", deepstream_views.DeepStreamHealthView.as_view(), name="deepstream-health"),
    path("streams/", deepstream_views.DeepStreamStreamsView.as_view(), name="deepstream-streams"),
]
