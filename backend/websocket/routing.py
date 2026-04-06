from django.urls import path

from . import consumers

websocket_urlpatterns = [
    path("ws/detections/", consumers.DetectionConsumer.as_asgi()),
    path("ws/cameras/status/", consumers.CameraStatusConsumer.as_asgi()),
    path("ws/alerts/", consumers.AlertConsumer.as_asgi()),
]
