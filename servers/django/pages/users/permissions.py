from rest_framework.permissions import BasePermission


class HasUsersPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewUser(HasUsersPerm):
    perm = "users.view_user"


class HasAddUser(HasUsersPerm):
    perm = "users.add_user"


class HasChangeUser(HasUsersPerm):
    perm = "users.change_user"


class HasDeleteUser(HasUsersPerm):
    perm = "users.delete_user"


class HasViewGroup(HasUsersPerm):
    perm = "users.view_group"


class HasAddGroup(HasUsersPerm):
    perm = "users.add_group"


class HasChangeGroup(HasUsersPerm):
    perm = "users.change_group"


class HasDeleteGroup(HasUsersPerm):
    perm = "users.delete_group"


class HasViewGroupOrUser(BasePermission):
    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = (
                user.is_superuser
                or user.has_perm("users.view_group")
                or user.has_perm("users.view_user")
            )
        return allowed
