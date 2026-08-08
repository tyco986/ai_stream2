from rest_framework.parsers import JSONParser
from rest_framework.views import APIView

from pages.pipelines.permissions import (
    HasAddPipeline,
    HasChangePipeline,
    HasDeletePipeline,
    HasViewPipeline,
)
from pages.pipelines.serializers import BatchIdsSerializer, GieTemplateBodySerializer
from pages.pipelines.services import GieTemplateService
from shared.http.response import api_success
from shared.pagination import PaginationService


class GieTemplateListCreateView(APIView):
    parser_classes = [JSONParser]

    def get_permissions(self):
        classes = [HasViewPipeline]
        if self.request.method == "POST":
            classes = [HasAddPipeline]
        return [item() for item in classes]

    def get(self, request):
        paging = PaginationService().parse(request.query_params)
        data = GieTemplateService().list_templates(
            search=request.query_params.get("search"),
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)

    def post(self, request):
        serializer = GieTemplateBodySerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = GieTemplateService().create(serializer.validated_data)
        return api_success(data, status=201)


class GieTemplateBatchDeleteView(APIView):
    permission_classes = [HasDeletePipeline]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = GieTemplateService().batch_delete(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class GieTemplateDetailView(APIView):
    parser_classes = [JSONParser]

    def get_permissions(self):
        mapping = {
            "GET": HasViewPipeline,
            "PUT": HasChangePipeline,
            "DELETE": HasDeletePipeline,
        }
        cls = mapping.get(self.request.method, HasViewPipeline)
        return [cls()]

    def get(self, request, gie_id):
        return api_success(GieTemplateService().get(gie_id))

    def put(self, request, gie_id):
        serializer = GieTemplateBodySerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = GieTemplateService().update(gie_id, serializer.validated_data)
        return api_success(data)

    def delete(self, request, gie_id):
        GieTemplateService().delete(gie_id)
        return api_success({})
