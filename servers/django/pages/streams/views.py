from rest_framework.parsers import FormParser, MultiPartParser
from rest_framework.views import APIView

from pages.streams.permissions import (
    HasAddStream,
    HasChangeStream,
    HasDeleteStream,
    HasViewStream,
)
from pages.streams.serializers import (
    BatchIdsSerializer,
    CreateStreamSerializer,
    GroupNameSerializer,
    PatchStreamSerializer,
    ProbeUrlSerializer,
    SetMembersSerializer,
)
from pages.streams.services import (
    GroupService,
    StreamLogService,
    StreamProbeService,
    StreamPublisherService,
    StreamService,
)
from shared.http.response import api_success
from shared.pagination import PaginationService


class GroupsTreeView(APIView):
    permission_classes = [HasViewStream]

    def get(self, request):
        return api_success(GroupService().build_tree())


class GroupsMapView(APIView):
    permission_classes = [HasViewStream]

    def get(self, request):
        return api_success(GroupService().groups_map())


class GroupsListView(APIView):
    permission_classes = [HasViewStream]

    def get(self, request):
        return api_success(GroupService().list_groups())


class GroupMembersView(APIView):
    def get_permissions(self):
        classes = [HasViewStream]
        if self.request.method == "PUT":
            classes = [HasChangeStream]
        return [item() for item in classes]

    def get(self, request, group_id):
        return api_success(GroupService().members(group_id))

    def put(self, request, group_id):
        serializer = SetMembersSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = GroupService().set_members(
            group_id,
            [str(item) for item in serializer.validated_data["stream_ids"]],
        )
        return api_success(data)


class GroupCandidatesView(APIView):
    permission_classes = [HasViewStream]

    def get(self, request, group_id):
        data = GroupService().candidates(
            group_id,
            search=request.query_params.get("search"),
        )
        return api_success(data)


class GroupCreateOrDetailView(APIView):
    def get_permissions(self):
        mapping = {
            "POST": HasAddStream,
            "PATCH": HasChangeStream,
            "DELETE": HasDeleteStream,
        }
        cls = mapping.get(self.request.method, HasViewStream)
        return [cls()]

    def post(self, request, group_id):
        serializer = GroupNameSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = GroupService().create(group_id, serializer.validated_data["name"])
        return api_success(data, status=201)

    def patch(self, request, group_id):
        serializer = GroupNameSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = GroupService().rename(group_id, serializer.validated_data["name"])
        return api_success(data)

    def delete(self, request, group_id):
        data = GroupService().delete(group_id)
        return api_success(data)


class StreamsMapView(APIView):
    permission_classes = [HasViewStream]

    def get(self, request):
        return api_success(StreamService().streams_map())


class StreamListCreateView(APIView):
    def get_permissions(self):
        classes = [HasViewStream]
        if self.request.method == "POST":
            classes = [HasAddStream]
        return [item() for item in classes]

    def get(self, request):
        paging = PaginationService().parse(request.query_params)
        data = StreamService().list_streams(
            group_id=request.query_params.get("group_id"),
            stream_id=request.query_params.get("stream_id"),
            search=request.query_params.get("search"),
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)

    def post(self, request):
        serializer = CreateStreamSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = StreamService().create(
            body["name"],
            body["url"],
            group_id=str(body["group_id"]) if "group_id" in body else None,
            enabled=body.get("enabled", True),
            recording=body.get("recording", False),
            resolution=body.get("resolution"),
            fps=body.get("fps"),
        )
        return api_success(data, status=201)


class StreamDetailView(APIView):
    def get_permissions(self):
        mapping = {
            "GET": HasViewStream,
            "PATCH": HasChangeStream,
            "DELETE": HasDeleteStream,
        }
        cls = mapping.get(self.request.method, HasViewStream)
        return [cls()]

    def get(self, request, stream_id):
        return api_success(StreamService().get(stream_id))

    def patch(self, request, stream_id):
        serializer = PatchStreamSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = StreamService().patch(
            stream_id,
            name=body.get("name"),
            group_id=str(body["group_id"]) if "group_id" in body else None,
            url=body.get("url"),
            enabled=body.get("enabled"),
            recording=body.get("recording"),
            resolution=body.get("resolution"),
            fps=body.get("fps"),
        )
        return api_success(data)

    def delete(self, request, stream_id):
        StreamService().delete(stream_id)
        return api_success({})


class StreamIdProbeView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request, stream_id):
        return api_success(StreamProbeService().probe_one(stream_id))


class StreamProbeView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request):
        serializer = ProbeUrlSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        return api_success(
            StreamProbeService().probe_url(serializer.validated_data["url"])
        )


class StreamLogsView(APIView):
    permission_classes = [HasViewStream]

    def get(self, request, stream_id):
        StreamService().resolve(stream_id)
        return api_success(StreamLogService().get_logs(stream_id))


class BatchDeleteView(APIView):
    permission_classes = [HasDeleteStream]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = StreamService().batch_delete(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class BatchEnableView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = StreamService().batch_enable(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class BatchDisableView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = StreamService().batch_disable(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class BatchRecordView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = StreamService().batch_record(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class BatchUnrecordView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = StreamService().batch_unrecord(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class BatchProbeView(APIView):
    permission_classes = [HasChangeStream]

    def post(self, request):
        serializer = BatchIdsSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = StreamProbeService().probe_many(
            [str(item) for item in serializer.validated_data["ids"]]
        )
        return api_success(data)


class StreamPublishersView(APIView):
    permission_classes = [HasAddStream]
    parser_classes = [MultiPartParser, FormParser]

    def post(self, request):
        upload = request.FILES.get("input")
        name = request.data.get("name")
        data = StreamPublisherService().publish(upload, name=name)
        return api_success(data, status=201)
