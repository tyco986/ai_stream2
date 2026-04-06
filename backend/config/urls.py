from django.contrib import admin
from django.urls import include, path
from drf_spectacular.views import SpectacularAPIView, SpectacularSwaggerView

urlpatterns = [
    path("admin/", admin.site.urls),
    # API v1
    path("api/v1/auth/", include("apps.accounts.urls")),
    path("api/v1/", include("apps.cameras.urls")),
    path("api/v1/", include("apps.detections.urls")),
    path("api/v1/", include("apps.alerts.urls")),
    path("api/v1/", include("apps.pipelines.urls")),
    path("api/v1/", include("apps.dashboard.urls")),
    # Health checks
    path("api/v1/health/", include("apps.dashboard.health_urls")),
    # DeepStream proxy
    path("api/v1/deepstream/", include("apps.cameras.deepstream_urls")),
    # OpenAPI
    path("api/schema/", SpectacularAPIView.as_view(), name="schema"),
    path("api/docs/", SpectacularSwaggerView.as_view(url_name="schema"), name="swagger-ui"),
]
