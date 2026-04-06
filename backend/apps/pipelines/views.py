from rest_framework import viewsets
from rest_framework.decorators import action

from common.exceptions import DeploymentError
from common.permissions import IsOperatorOrAbove, IsViewer, OrganizationFilterMixin
from common.response import success_response
from services.pipeline_deployer import PipelineDeployer

from .models import AIModel, PipelineProfile
from .serializers import AIModelSerializer, PipelineProfileSerializer


class AIModelViewSet(OrganizationFilterMixin, viewsets.ModelViewSet):
    queryset = AIModel.objects.all()
    serializer_class = AIModelSerializer

    def get_permissions(self):
        if self.action in ("list", "retrieve"):
            return [IsViewer()]
        return [IsOperatorOrAbove()]


class PipelineProfileViewSet(OrganizationFilterMixin, viewsets.ModelViewSet):
    queryset = PipelineProfile.objects.select_related("detector", "tracker").all()
    serializer_class = PipelineProfileSerializer

    def get_permissions(self):
        if self.action in ("list", "retrieve"):
            return [IsViewer()]
        return [IsOperatorOrAbove()]

    @action(detail=True, methods=["post"])
    def deploy(self, request, pk=None):
        profile = self.get_object()
        deployer = PipelineDeployer()
        try:
            deployer.deploy(profile)
        except Exception as e:
            raise DeploymentError(str(e))
        serializer = PipelineProfileSerializer(profile)
        return success_response(serializer.data, message="deployment initiated")
