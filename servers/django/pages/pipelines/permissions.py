from rest_framework.permissions import BasePermission


class HasPipelinesPerm(BasePermission):
    perm = ""

    def has_permission(self, request, view):
        user = request.user
        allowed = False
        if user and getattr(user, "is_authenticated", False):
            allowed = user.is_superuser or user.has_perm(self.perm)
        return allowed


class HasViewPipeline(HasPipelinesPerm):
    perm = "pipelines.view_pipeline"


class HasAddPipeline(HasPipelinesPerm):
    perm = "pipelines.add_pipeline"


class HasChangePipeline(HasPipelinesPerm):
    perm = "pipelines.change_pipeline"


class HasDeletePipeline(HasPipelinesPerm):
    perm = "pipelines.delete_pipeline"
