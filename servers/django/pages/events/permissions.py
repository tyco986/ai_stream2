from rest_framework.permissions import BasePermission


class HasEventsPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewEvent(HasEventsPerm):
    perm = "users.view_event"


class HasChangeEvent(HasEventsPerm):
    perm = "users.change_event"
