from rest_framework.parsers import FormParser, JSONParser, MultiPartParser
from rest_framework.views import APIView

from pages.models_page.permissions import (
    HasAddModel,
    HasChangeModel,
    HasDeleteModel,
    HasViewModel,
)
from pages.models_page.serializers import (
    BatchDeleteModelsSerializer,
    CreateModelSerializer,
)
from pages.models_page.services import (
    BuildOrchestrator,
    ModelLogService,
    ModelService,
    ModelTypeService,
)
from shared.http.exceptions import AppError
from shared.http.response import api_success
from shared.pagination import PaginationService


class ModelMapView(APIView):
    permission_classes = [HasViewModel]

    def get(self, request):
        return api_success(ModelService().maps())


class ModelTypesView(APIView):
    permission_classes = [HasViewModel]

    def get(self, request):
        return api_success(ModelTypeService().list_types())


class ModelListCreateView(APIView):
    parser_classes = [MultiPartParser, FormParser, JSONParser]

    def get_permissions(self):
        classes = [HasViewModel]
        if self.request.method == "POST":
            classes = [HasAddModel]
        return [item() for item in classes]

    def get(self, request):
        paging = PaginationService().parse(request.query_params)
        data = ModelService().list_models(
            search=request.query_params.get("search"),
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)

    def post(self, request):
        serializer = CreateModelSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = ModelService().create(
            body["name"],
            body["batch_mode"],
            body["batch_size"],
            body["family"],
            precision=body.get("precision"),
            optimization_level=body["optimization_level"],
            conf=body.get("conf"),
            iou=body.get("iou"),
            upload=request.FILES.get("source_file"),
        )
        return api_success(data, status=201)


class ModelBatchDeleteView(APIView):
    permission_classes = [HasDeleteModel]

    def post(self, request):
        serializer = BatchDeleteModelsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = ModelService().batch_delete(serializer.validated_data["ids"])
        return api_success(data)


class ModelDetailView(APIView):
    parser_classes = [MultiPartParser, FormParser, JSONParser]

    def get_permissions(self):
        mapping = {
            "GET": HasViewModel,
            "PUT": HasChangeModel,
            "DELETE": HasDeleteModel,
        }
        cls = mapping.get(self.request.method, HasViewModel)
        return [cls()]

    def get(self, request, model_id):
        return api_success(ModelService().get(model_id))

    def put(self, request, model_id):
        serializer = CreateModelSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = ModelService().update(
            model_id,
            body["name"],
            body["batch_mode"],
            body["batch_size"],
            body["family"],
            precision=body.get("precision"),
            optimization_level=body["optimization_level"],
            conf=body.get("conf"),
            iou=body.get("iou"),
            upload=request.FILES.get("source_file"),
        )
        return api_success(data)

    def delete(self, request, model_id):
        return api_success(ModelService().delete(model_id))


class ModelBuildView(APIView):
    permission_classes = [HasChangeModel]

    def post(self, request, model_id):
        return api_success(BuildOrchestrator().start(model_id))


class ModelBuildStatusView(APIView):
    permission_classes = [HasViewModel]

    def get(self, request, model_id):
        return api_success(BuildOrchestrator().get_status(model_id))


class ModelLogsView(APIView):
    permission_classes = [HasViewModel]

    def get(self, request, model_id):
        raw_tail = request.query_params.get("tail")
        tail = None
        if raw_tail is not None:
            try:
                tail = int(raw_tail)
            except (TypeError, ValueError) as exc:
                raise AppError("Invalid tail", status_code=400) from exc
        return api_success(ModelLogService().get_logs(model_id, tail=tail))
