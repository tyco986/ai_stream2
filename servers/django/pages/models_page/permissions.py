from rest_framework.permissions import BasePermission


class HasModelsPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewModel(HasModelsPerm):
    perm = "models.view_model"


class HasAddModel(HasModelsPerm):
    perm = "models.add_model"


class HasChangeModel(HasModelsPerm):
    perm = "models.change_model"


class HasDeleteModel(HasModelsPerm):
    perm = "models.delete_model"
