from django.http import FileResponse
from rest_framework.parsers import FormParser, JSONParser, MultiPartParser
from rest_framework.views import APIView

from pages.preview.permissions import (
    HasAddPreview,
    HasChangePreview,
    HasDeletePreview,
    HasViewPreview,
)
from pages.preview.serializers import (
    ActiveLayoutSerializer,
    BatchDeleteLayoutsSerializer,
    CreateLayoutSerializer,
    PatchLayoutSerializer,
)
from pages.preview.services import ActiveLayoutService, LayoutPresetService, ShotService
from shared.http.response import api_success


class ActiveLayoutView(APIView):
    def get_permissions(self):
        classes = [HasViewPreview]
        if self.request.method == "PUT":
            classes = [HasChangePreview]
        return [item() for item in classes]

    def get(self, request):
        return api_success(ActiveLayoutService().get_preset_id(request.user))

    def put(self, request):
        serializer = ActiveLayoutSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = ActiveLayoutService().set_preset_id(
            request.user,
            serializer.validated_data["preset_id"],
        )
        return api_success(data)


class LayoutListCreateView(APIView):
    def get_permissions(self):
        classes = [HasViewPreview]
        if self.request.method == "POST":
            classes = [HasAddPreview]
        return [item() for item in classes]

    def get(self, request):
        data = LayoutPresetService().list_presets(
            request.user,
            search=request.query_params.get("search"),
        )
        return api_success(data)

    def post(self, request):
        serializer = CreateLayoutSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = LayoutPresetService().create(
            body["name"],
            body["layout"],
            body["view_mode"],
            body["slots"],
        )
        return api_success(data, status=201)


class LayoutsMapView(APIView):
    permission_classes = [HasViewPreview]

    def get(self, request):
        return api_success(LayoutPresetService().layouts_map())


class LayoutBatchDeleteView(APIView):
    permission_classes = [HasDeletePreview]

    def post(self, request):
        serializer = BatchDeleteLayoutsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = LayoutPresetService().batch_delete(
            request.user,
            [str(item) for item in serializer.validated_data["ids"]],
        )
        return api_success(data)


class LayoutShotView(APIView):
    parser_classes = [MultiPartParser, FormParser, JSONParser]

    def get_permissions(self):
        classes = [HasViewPreview]
        if self.request.method == "PUT":
            classes = [HasChangePreview]
        return [item() for item in classes]

    def get(self, request, preset_id):
        path = ShotService().get_shot_path(preset_id)
        return FileResponse(path.open("rb"), content_type="image/jpeg")

    def put(self, request, preset_id):
        upload = request.FILES.get("file")
        data = ShotService().put_shot(preset_id, upload)
        return api_success(data)


class LayoutDetailView(APIView):
    def get_permissions(self):
        mapping = {
            "GET": HasViewPreview,
            "PATCH": HasChangePreview,
            "DELETE": HasDeletePreview,
        }
        cls = mapping.get(self.request.method, HasViewPreview)
        return [cls()]

    def get(self, request, preset_id):
        return api_success(LayoutPresetService().get(preset_id))

    def patch(self, request, preset_id):
        serializer = PatchLayoutSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = LayoutPresetService().patch(
            preset_id,
            name=body.get("name"),
            layout=body.get("layout"),
            view_mode=body.get("view_mode"),
            slots=body.get("slots"),
        )
        return api_success(data)

    def delete(self, request, preset_id):
        data = LayoutPresetService().delete(request.user, preset_id)
        return api_success(data)
