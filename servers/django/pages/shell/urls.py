from django.urls import path

from pages.shell.views import (
    LogoutView,
    MeView,
    SettingsView,
    SiteConfigExportView,
    SiteConfigImportView,
    SiteConfigVersionApplyView,
    SiteConfigVersionExportView,
    SiteConfigVersionImportView,
    SiteConfigVersionListCreateView,
)

urlpatterns = [
    path("me", MeView.as_view(), name="shell-me"),
    path("logout", LogoutView.as_view(), name="shell-logout"),
    path("settings", SettingsView.as_view(), name="shell-settings"),
    path("site-config/export", SiteConfigExportView.as_view(), name="shell-site-export"),
    path("site-config/import", SiteConfigImportView.as_view(), name="shell-site-import"),
    path(
        "site-config/versions",
        SiteConfigVersionListCreateView.as_view(),
        name="shell-site-versions",
    ),
    path(
        "site-config/versions/<uuid:version_id>/import",
        SiteConfigVersionImportView.as_view(),
        name="shell-site-version-import",
    ),
    path(
        "site-config/versions/<uuid:version_id>/export",
        SiteConfigVersionExportView.as_view(),
        name="shell-site-version-export",
    ),
    path(
        "site-config/versions/<uuid:version_id>/apply",
        SiteConfigVersionApplyView.as_view(),
        name="shell-site-version-apply",
    ),
]
