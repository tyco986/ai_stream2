from rest_framework.parsers import FormParser, JSONParser, MultiPartParser
from rest_framework.views import APIView

from pages.pipelines.permissions import (
    HasAddPipeline,
    HasChangePipeline,
    HasDeletePipeline,
    HasViewPipeline,
)
from pages.pipelines.serializers import (
    BatchIdsSerializer,
    PipelineBodySerializer,
    PipelineSchemaSerializer,
)
from pages.pipelines.services import (
    PipelineLogService,
    PipelineService,
    StartStopOrchestrator,
)
from pages.pipelines.type_registry import TypeRegistry
from shared.http.exceptions import AppError
from shared.http.response import api_success
from shared.pagination import PaginationService


class PipelineTypesView(APIView):
    permission_classes = [HasViewPipeline]

    def get(self, request):
        return api_success(TypeRegistry.list_types())


class PipelineSchemaView(APIView):
    permission_classes = [HasViewPipeline]

    def post(self, request):
        serializer = PipelineSchemaSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        pipeline_type = serializer.validated_data["pipeline_type"]
        schema = TypeRegistry.get_schema(pipeline_type)
        if schema is None:
            raise AppError(f"Unknown pipeline type: {pipeline_type}", status_code=404)
        return api_success(schema)


class PipelineMapView(APIView):
    permission_classes = [HasViewPipeline]

    def get(self, request):
        return api_success(PipelineService().pipelines_map())


class PipelineListCreateView(APIView):
    parser_classes = [JSONParser]

    def get_permissions(self):
        classes = [HasViewPipeline]
        if self.request.method == "POST":
            classes = [HasAddPipeline]
        return [item() for item in classes]

    def get(self, request):
        paging = PaginationService().parse(request.query_params)
        data = PipelineService().list_pipelines(
            search=request.query_params.get("search"),
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)

    def post(self, request):
        serializer = PipelineBodySerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = PipelineService().create(serializer.validated_data)
        return api_success(data, status=201)


class PipelineBatchDeleteView(APIView):
    permission_classes = [HasDeletePipeline]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = PipelineService().batch_delete(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class PipelineDetailView(APIView):
    parser_classes = [JSONParser]

    def get_permissions(self):
        mapping = {
            "GET": HasViewPipeline,
            "PUT": HasChangePipeline,
            "DELETE": HasDeletePipeline,
        }
        cls = mapping.get(self.request.method, HasViewPipeline)
        return [cls()]

    def get(self, request, pipeline_id):
        return api_success(PipelineService().get(pipeline_id))

    def put(self, request, pipeline_id):
        serializer = PipelineBodySerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = PipelineService().update(pipeline_id, serializer.validated_data)
        return api_success(data)

    def delete(self, request, pipeline_id):
        PipelineService().delete(pipeline_id)
        return api_success({})


class PipelineStartView(APIView):
    permission_classes = [HasChangePipeline]

    def post(self, request, pipeline_id):
        return api_success(StartStopOrchestrator().start(pipeline_id))


class PipelineStopView(APIView):
    permission_classes = [HasChangePipeline]

    def post(self, request, pipeline_id):
        return api_success(StartStopOrchestrator().stop(pipeline_id))


class PipelineStatusView(APIView):
    permission_classes = [HasViewPipeline]

    def get(self, request, pipeline_id):
        return api_success(StartStopOrchestrator().get_status(pipeline_id))


class PipelineLogsView(APIView):
    permission_classes = [HasViewPipeline]

    def get(self, request, pipeline_id):
        PipelineService().resolve(pipeline_id)
        tail = request.query_params.get("tail")
        offset = request.query_params.get("offset")
        tail_value = int(tail) if tail is not None and tail != "" else None
        offset_value = int(offset) if offset is not None and offset != "" else None
        data = PipelineLogService().get_logs(
            pipeline_id,
            tail=tail_value,
            offset=offset_value,
        )
        return api_success(data)
