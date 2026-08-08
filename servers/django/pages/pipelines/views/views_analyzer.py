from rest_framework.parsers import FormParser, JSONParser, MultiPartParser
from rest_framework.views import APIView

from pages.pipelines.permissions import (
    HasAddPipeline,
    HasChangePipeline,
    HasDeletePipeline,
    HasViewPipeline,
)
from pages.pipelines.serializers import (
    AnalyzerSourceStreamSerializer,
    AnalyzerTemplateBodySerializer,
    BatchIdsSerializer,
)
from pages.pipelines.services import AnalyzerTemplateService
from shared.http.response import api_success
from shared.pagination import PaginationService


class AnalyzerTemplateListCreateView(APIView):
    parser_classes = [JSONParser]

    def get_permissions(self):
        classes = [HasViewPipeline]
        if self.request.method == "POST":
            classes = [HasAddPipeline]
        return [item() for item in classes]

    def get(self, request):
        paging = PaginationService().parse(request.query_params)
        data = AnalyzerTemplateService().list_templates(
            search=request.query_params.get("search"),
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)

    def post(self, request):
        serializer = AnalyzerTemplateBodySerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = AnalyzerTemplateService().create(serializer.validated_data)
        return api_success(data, status=201)


class AnalyzerTemplateBatchDeleteView(APIView):
    permission_classes = [HasDeletePipeline]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = AnalyzerTemplateService().batch_delete(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class AnalyzerTemplateDetailView(APIView):
    parser_classes = [JSONParser]

    def get_permissions(self):
        mapping = {
            "GET": HasViewPipeline,
            "PUT": HasChangePipeline,
            "DELETE": HasDeletePipeline,
        }
        cls = mapping.get(self.request.method, HasViewPipeline)
        return [cls()]

    def get(self, request, analyzer_template_id):
        return api_success(AnalyzerTemplateService().get(analyzer_template_id))

    def put(self, request, analyzer_template_id):
        serializer = AnalyzerTemplateBodySerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = AnalyzerTemplateService().update(
            analyzer_template_id,
            serializer.validated_data,
        )
        return api_success(data)

    def delete(self, request, analyzer_template_id):
        AnalyzerTemplateService().delete(analyzer_template_id)
        return api_success({})


class AnalyzerSourceFileView(APIView):
    parser_classes = [MultiPartParser, FormParser]
    permission_classes = [HasChangePipeline]

    def post(self, request, analyzer_template_id):
        upload = request.FILES.get("file")
        data = AnalyzerTemplateService().upload_source_file(analyzer_template_id, upload)
        return api_success(data)


class AnalyzerSourceStreamView(APIView):
    parser_classes = [JSONParser]
    permission_classes = [HasChangePipeline]

    def post(self, request, analyzer_template_id):
        serializer = AnalyzerSourceStreamSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = AnalyzerTemplateService().capture_source_stream(
            analyzer_template_id,
            serializer.validated_data["stream_id"],
        )
        return api_success(data)
