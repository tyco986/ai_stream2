from rest_framework.permissions import BasePermission


class HasPreviewPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewPreview(HasPreviewPerm):
    perm = "previews.view_preview"


class HasAddPreview(HasPreviewPerm):
    perm = "previews.add_preview"


class HasChangePreview(HasPreviewPerm):
    perm = "previews.change_preview"


class HasDeletePreview(HasPreviewPerm):
    perm = "previews.delete_preview"
