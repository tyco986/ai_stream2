from asgiref.sync import async_to_sync
from rest_framework import viewsets
from rest_framework.decorators import action

from common.exceptions import DeepStreamUnavailableError
from common.permissions import IsOperatorOrAbove, IsViewer, OrganizationFilterMixin
from common.response import success_response
from services.deepstream_client import deepstream_client

from .models import AnalyticsZone, Camera, CameraGroup
from .serializers import (
    AnalyticsZoneSerializer,
    CameraGroupSerializer,
    CameraReadSerializer,
    CameraWriteSerializer,
)


class CameraViewSet(OrganizationFilterMixin, viewsets.ModelViewSet):
    queryset = Camera.objects.select_related("group", "organization").all()

    def get_serializer_class(self):
        if self.action in ("create", "partial_update", "update"):
            return CameraWriteSerializer
        return CameraReadSerializer

    def get_permissions(self):
        if self.action in ("list", "retrieve"):
            return [IsViewer()]
        return [IsOperatorOrAbove()]

    def perform_destroy(self, instance):
        instance.is_deleted = True
        instance.save(update_fields=["is_deleted", "updated_at"])

    @action(detail=True, methods=["post"], url_path="start-stream")
    def start_stream(self, request, pk=None):
        camera = self.get_object()
        if camera.status in ("online", "connecting"):
            serializer = CameraReadSerializer(camera)
            return success_response(serializer.data, message="already streaming")

        try:
            async_to_sync(deepstream_client.add_stream)(
                camera.uid, camera.name, camera.rtsp_url,
            )
        except Exception:
            raise DeepStreamUnavailableError()

        camera.status = Camera.Status.CONNECTING
        camera.save(update_fields=["status", "updated_at"])
        serializer = CameraReadSerializer(camera)
        return success_response(serializer.data, message="stream starting")

    @action(detail=True, methods=["post"], url_path="stop-stream")
    def stop_stream(self, request, pk=None):
        camera = self.get_object()
        if camera.status == "offline":
            serializer = CameraReadSerializer(camera)
            return success_response(serializer.data, message="already offline")

        try:
            async_to_sync(deepstream_client.remove_stream)(
                camera.uid, camera.rtsp_url,
            )
        except Exception:
            raise DeepStreamUnavailableError()

        camera.status = Camera.Status.OFFLINE
        camera.save(update_fields=["status", "updated_at"])
        serializer = CameraReadSerializer(camera)
        return success_response(serializer.data, message="stream stopped")

    @action(detail=True, methods=["get"], url_path="pipeline")
    def get_pipeline(self, request, pk=None):
        camera = self.get_object()
        from apps.pipelines.serializers import CameraModelBindingSerializer
        try:
            binding = camera.model_binding
            serializer = CameraModelBindingSerializer(binding)
            return success_response(serializer.data)
        except Camera.model_binding.RelatedObjectDoesNotExist:
            return success_response(None, message="no pipeline bound")

    @get_pipeline.mapping.put
    def set_pipeline(self, request, pk=None):
        camera = self.get_object()
        from apps.pipelines.models import CameraModelBinding, PipelineProfile
        profile_id = request.data.get("pipeline_profile_id")
        profile = PipelineProfile.objects.get(
            id=profile_id,
            organization=request.user.organization,
        )
        binding, _ = CameraModelBinding.objects.update_or_create(
            camera=camera,
            defaults={"pipeline_profile": profile, "is_enabled": True},
        )
        from apps.pipelines.serializers import CameraModelBindingSerializer
        serializer = CameraModelBindingSerializer(binding)
        return success_response(serializer.data, message="pipeline bound")


class CameraGroupViewSet(OrganizationFilterMixin, viewsets.ModelViewSet):
    queryset = CameraGroup.objects.select_related("organization").all()
    serializer_class = CameraGroupSerializer

    def get_permissions(self):
        if self.action in ("list", "retrieve"):
            return [IsViewer()]
        return [IsOperatorOrAbove()]


class AnalyticsZoneViewSet(viewsets.ModelViewSet):
    serializer_class = AnalyticsZoneSerializer

    def get_permissions(self):
        if self.action in ("list", "retrieve"):
            return [IsViewer()]
        return [IsOperatorOrAbove()]

    def get_queryset(self):
        camera_id = self.kwargs["camera_pk"]
        return AnalyticsZone.objects.filter(
            camera_id=camera_id,
            camera__organization=self.request.user.organization,
        )

    def perform_create(self, serializer):
        camera_id = self.kwargs["camera_pk"]
        camera = Camera.objects.get(
            id=camera_id,
            organization=self.request.user.organization,
            is_deleted=False,
        )
        serializer.save(camera=camera)
