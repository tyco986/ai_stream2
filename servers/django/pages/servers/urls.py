from django.urls import path

from pages.servers.views import (
    ServerHealthView,
    ServerListView,
    ServerLogsView,
    ServerRefreshView,
    ServerRestartView,
)

urlpatterns = [
    path("refresh", ServerRefreshView.as_view(), name="servers-refresh"),
    path(
        "<str:server_id>/health",
        ServerHealthView.as_view(),
        name="servers-health",
    ),
    path(
        "<str:server_id>/restart",
        ServerRestartView.as_view(),
        name="servers-restart",
    ),
    path(
        "<str:server_id>/logs",
        ServerLogsView.as_view(),
        name="servers-logs",
    ),
    path("", ServerListView.as_view(), name="servers-list"),
]
