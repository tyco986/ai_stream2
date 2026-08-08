from django.conf import settings
from django.urls import include, path

from pages.events.views import EventMediaView
from pages.recordings.views import RecordingMediaView

backend_prefix = f"{settings.PROJECT_NAME}/backend"
media_prefix = f"{settings.PROJECT_NAME}/media"

urlpatterns = [
    path(f"{backend_prefix}/login/", include("pages.login.urls")),
    path(f"{backend_prefix}/shell/", include("pages.shell.urls")),
    path(f"{backend_prefix}/users/", include("pages.users.urls")),
    path(f"{backend_prefix}/streams/", include("pages.streams.urls")),
    path(f"{backend_prefix}/preview/", include("pages.preview.urls")),
    path(f"{backend_prefix}/recordings/", include("pages.recordings.urls")),
    path(f"{backend_prefix}/servers/", include("pages.servers.urls")),
    path(f"{backend_prefix}/models/", include("pages.models_page.urls")),
    path(f"{backend_prefix}/pipelines/", include("pages.pipelines.urls_pipelines")),
    path(f"{backend_prefix}/gie-templates/", include("pages.pipelines.urls_gie")),
    path(
        f"{backend_prefix}/analyzer-templates/",
        include("pages.pipelines.urls_analyzer"),
    ),
    path(f"{backend_prefix}/events/", include("pages.events.urls")),
    path(
        f"{media_prefix}/recordings/<path:rel>",
        RecordingMediaView.as_view(),
        name="recordings-media",
    ),
    path(
        f"{media_prefix}/events/<path:rel>",
        EventMediaView.as_view(),
        name="events-media",
    ),
]
