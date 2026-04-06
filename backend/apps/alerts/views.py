from django.utils.timezone import now
from rest_framework import mixins, viewsets
from rest_framework.decorators import action

from common.exceptions import InvalidStateTransitionError
from common.permissions import IsOperatorOrAbove, IsViewer, OrganizationFilterMixin
from common.response import success_response

from .models import Alert, AlertRule
from .serializers import AlertRuleSerializer, AlertSerializer


class AlertRuleViewSet(OrganizationFilterMixin, viewsets.ModelViewSet):
    queryset = AlertRule.objects.prefetch_related("cameras").all()
    serializer_class = AlertRuleSerializer

    def get_permissions(self):
        if self.action in ("list", "retrieve"):
            return [IsViewer()]
        return [IsOperatorOrAbove()]


class AlertViewSet(
    OrganizationFilterMixin,
    mixins.ListModelMixin,
    mixins.RetrieveModelMixin,
    viewsets.GenericViewSet,
):
    queryset = Alert.objects.select_related("rule", "camera").all()
    serializer_class = AlertSerializer
    permission_classes = [IsViewer]

    @action(detail=True, methods=["post"], permission_classes=[IsOperatorOrAbove])
    def acknowledge(self, request, pk=None):
        alert = self.get_object()
        if alert.status != Alert.Status.PENDING:
            raise InvalidStateTransitionError(alert.status, "acknowledge")
        alert.status = Alert.Status.ACKNOWLEDGED
        alert.acknowledged_by = request.user
        alert.acknowledged_at = now()
        alert.save(update_fields=["status", "acknowledged_by", "acknowledged_at", "updated_at"])
        serializer = AlertSerializer(alert)
        return success_response(serializer.data, message="acknowledged")

    @action(detail=True, methods=["post"], permission_classes=[IsOperatorOrAbove])
    def resolve(self, request, pk=None):
        alert = self.get_object()
        if alert.status not in (Alert.Status.PENDING, Alert.Status.ACKNOWLEDGED):
            raise InvalidStateTransitionError(alert.status, "resolve")
        alert.status = Alert.Status.RESOLVED
        alert.resolved_by = request.user
        alert.resolved_at = now()
        alert.save(update_fields=["status", "resolved_by", "resolved_at", "updated_at"])
        serializer = AlertSerializer(alert)
        return success_response(serializer.data, message="resolved")
