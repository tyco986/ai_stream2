from rest_framework.permissions import BasePermission


class HasStreamsPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewStream(HasStreamsPerm):
    perm = "streams.view_stream"


class HasAddStream(HasStreamsPerm):
    perm = "streams.add_stream"


class HasChangeStream(HasStreamsPerm):
    perm = "streams.change_stream"


class HasDeleteStream(HasStreamsPerm):
    perm = "streams.delete_stream"
