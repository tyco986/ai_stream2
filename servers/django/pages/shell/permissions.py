from rest_framework.permissions import BasePermission


class IsAuthenticatedSession(BasePermission):
    def has_permission(self, request, view):
        user = request.user
        return bool(user and getattr(user, "is_authenticated", False))


class HasExportSiteConfig(BasePermission):
    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm("users.export_site_config")
        return allowed


class HasImportSiteConfig(BasePermission):
    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm("users.import_site_config")
        return allowed
