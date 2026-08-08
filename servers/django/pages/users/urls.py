from django.urls import path

from pages.users.views import (
    GroupDetailView,
    GroupListCreateView,
    PermissionCatalogView,
    UserBatchDeleteView,
    UserDetailView,
    UserListCreateView,
    UserLogsView,
)

urlpatterns = [
    path(
        "permissions/catalog",
        PermissionCatalogView.as_view(),
        name="users-permissions-catalog",
    ),
    path("groups", GroupListCreateView.as_view(), name="users-groups"),
    path(
        "groups/<uuid:group_id>",
        GroupDetailView.as_view(),
        name="users-group-detail",
    ),
    path("batch/delete", UserBatchDeleteView.as_view(), name="users-batch-delete"),
    path("", UserListCreateView.as_view(), name="users-list"),
    path("<uuid:user_id>/logs", UserLogsView.as_view(), name="users-logs"),
    path("<uuid:user_id>", UserDetailView.as_view(), name="users-detail"),
]
