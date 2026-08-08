from django.http import HttpResponse
from rest_framework.parsers import FormParser, JSONParser, MultiPartParser
from rest_framework.views import APIView

from pages.shell.permissions import (
    HasExportSiteConfig,
    HasImportSiteConfig,
    IsAuthenticatedSession,
)
from pages.shell.serializers import (
    PageSettingsSerializer,
    SiteConfigVersionCreateSerializer,
    SiteConfigVersionSerializer,
)
from pages.shell.services import (
    LogoutService,
    SessionUserService,
    SettingsService,
    SiteConfigOrchestrator,
)
from shared.http.exceptions import AppError
from shared.http.response import api_success


class MeView(APIView):
    permission_classes = [IsAuthenticatedSession]

    def get(self, request):
        data = SessionUserService().get_session_user(request.user)
        return api_success(data)


class LogoutView(APIView):
    permission_classes = [IsAuthenticatedSession]

    def post(self, request):
        LogoutService().logout(request)
        return api_success({})


class SettingsView(APIView):
    permission_classes = [IsAuthenticatedSession]

    def get(self, request):
        data = SettingsService().get_mode(request.user)
        return api_success(data)

    def put(self, request):
        serializer = PageSettingsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = SettingsService().set_mode(request.user, serializer.validated_data["mode"])
        return api_success(data)


class SiteConfigExportView(APIView):
    permission_classes = [HasExportSiteConfig]

    def post(self, request):
        ciphertext = SiteConfigOrchestrator().export_current(request.user)
        response = HttpResponse(ciphertext, content_type="application/octet-stream")
        response["Content-Disposition"] = 'attachment; filename="site-config.age"'
        return response


class SiteConfigImportView(APIView):
    permission_classes = [HasImportSiteConfig]
    parser_classes = [MultiPartParser, FormParser]

    def post(self, request):
        upload = request.FILES.get("file")
        if upload is None:
            raise AppError("Missing file", status_code=400)
        ticket = request.headers.get("X-Auth-Ticket", "")
        data = SiteConfigOrchestrator().import_current(
            request.user,
            upload.read(),
            ticket,
        )
        return api_success(data)


class SiteConfigVersionListCreateView(APIView):
    def get_permissions(self):
        return [HasExportSiteConfig()]

    def get(self, request):
        data = SiteConfigOrchestrator().list_versions()
        return api_success(data)

    def post(self, request):
        serializer = SiteConfigVersionCreateSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = SiteConfigOrchestrator().backup_version(
            request.user,
            serializer.validated_data["version"],
            serializer.validated_data.get("description"),
        )
        return api_success(SiteConfigVersionSerializer(data).data)


class SiteConfigVersionImportView(APIView):
    permission_classes = [HasImportSiteConfig]
    parser_classes = [MultiPartParser, FormParser]

    def post(self, request, version_id):
        upload = request.FILES.get("file")
        if upload is None:
            raise AppError("Missing file", status_code=400)
        ticket = request.headers.get("X-Auth-Ticket", "")
        data = SiteConfigOrchestrator().import_version_payload(
            request.user,
            version_id,
            upload.read(),
            ticket,
        )
        return api_success(SiteConfigVersionSerializer(data).data)


class SiteConfigVersionExportView(APIView):
    permission_classes = [HasExportSiteConfig]

    def post(self, request, version_id):
        ciphertext = SiteConfigOrchestrator().export_version(request.user, version_id)
        response = HttpResponse(ciphertext, content_type="application/octet-stream")
        response["Content-Disposition"] = (
            f'attachment; filename="site-config-{version_id}.age"'
        )
        return response


class SiteConfigVersionApplyView(APIView):
    permission_classes = [HasImportSiteConfig]
    parser_classes = [JSONParser, FormParser, MultiPartParser]

    def post(self, request, version_id):
        ticket = request.headers.get("X-Auth-Ticket", "")
        data = SiteConfigOrchestrator().apply_version(request.user, version_id, ticket)
        return api_success(data)
