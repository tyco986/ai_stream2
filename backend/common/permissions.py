from rest_framework.permissions import BasePermission


class IsAdmin(BasePermission):
    def has_permission(self, request, view):
        return (
            request.user
            and request.user.is_authenticated
            and request.user.role == "admin"
        )


class IsOperatorOrAbove(BasePermission):
    def has_permission(self, request, view):
        return (
            request.user
            and request.user.is_authenticated
            and request.user.role in ("admin", "operator")
        )


class IsViewer(BasePermission):
    """Any authenticated user (admin/operator/viewer) can view."""

    def has_permission(self, request, view):
        return request.user and request.user.is_authenticated


class OrganizationFilterMixin:
    """所有涉及多租户数据的 ViewSet 必须混入此 Mixin。

    安全警告：遗漏此 Mixin 会导致跨租户数据泄漏。
    """

    def get_queryset(self):
        return super().get_queryset().filter(
            organization=self.request.user.organization
        )
