from rest_framework.permissions import BasePermission


class HasServersPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewServer(HasServersPerm):
    perm = "users.view_server"


class HasChangeServer(HasServersPerm):
    perm = "users.change_server"
