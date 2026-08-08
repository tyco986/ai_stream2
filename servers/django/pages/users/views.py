from rest_framework.views import APIView

from pages.users.permissions import (
    HasAddGroup,
    HasAddUser,
    HasChangeGroup,
    HasChangeUser,
    HasDeleteGroup,
    HasDeleteUser,
    HasViewGroup,
    HasViewGroupOrUser,
    HasViewUser,
)
from pages.users.serializers import (
    BatchDeleteUsersSerializer,
    CreateGroupSerializer,
    CreateUserSerializer,
    PatchGroupSerializer,
    PatchUserSerializer,
)
from pages.users.services import CatalogService, GroupService, UserService
from shared.http.response import api_success
from shared.pagination import PaginationService


class PermissionCatalogView(APIView):
    permission_classes = [HasViewGroup]

    def get(self, request):
        data = CatalogService().build()
        return api_success(data)


class GroupListCreateView(APIView):
    def get_permissions(self):
        classes = [HasViewGroupOrUser]
        if self.request.method == "POST":
            classes = [HasAddGroup]
        return [item() for item in classes]

    def get(self, request):
        data = GroupService().list_groups()
        return api_success(data)

    def post(self, request):
        serializer = CreateGroupSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = GroupService().create(
            body["name"],
            body.get("permission_ids"),
        )
        return api_success(data, status=201)


class GroupDetailView(APIView):
    def get_permissions(self):
        mapping = {
            "GET": HasViewGroup,
            "PATCH": HasChangeGroup,
            "DELETE": HasDeleteGroup,
        }
        cls = mapping.get(self.request.method, HasViewGroup)
        return [cls()]

    def get(self, request, group_id):
        data = GroupService().get(group_id)
        return api_success(data)

    def patch(self, request, group_id):
        serializer = PatchGroupSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = GroupService().patch(
            group_id,
            name=body.get("name"),
            permission_ids=body.get("permission_ids"),
        )
        return api_success(data)

    def delete(self, request, group_id):
        GroupService().delete(group_id)
        return api_success({})


class UserListCreateView(APIView):
    def get_permissions(self):
        classes = [HasViewUser]
        if self.request.method == "POST":
            classes = [HasAddUser]
        return [item() for item in classes]

    def get(self, request):
        paging = PaginationService().parse(request.query_params)
        data = UserService().list_users(
            search=request.query_params.get("search"),
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)

    def post(self, request):
        serializer = CreateUserSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        data = UserService().create(
            request.user,
            body["username"],
            body["password"],
            is_active=body.get("is_active", True),
            group_ids=[str(item) for item in body.get("group_ids", [])],
        )
        return api_success(data, status=201)


class UserDetailView(APIView):
    def get_permissions(self):
        mapping = {
            "GET": HasViewUser,
            "PATCH": HasChangeUser,
            "DELETE": HasDeleteUser,
        }
        cls = mapping.get(self.request.method, HasViewUser)
        return [cls()]

    def get(self, request, user_id):
        data = UserService().get(user_id)
        return api_success(data)

    def patch(self, request, user_id):
        serializer = PatchUserSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        body = serializer.validated_data
        group_ids = body.get("group_ids")
        data = UserService().patch(
            request.user,
            user_id,
            username=body.get("username"),
            password=body.get("password"),
            is_active=body.get("is_active"),
            group_ids=(
                [str(item) for item in group_ids] if group_ids is not None else None
            ),
        )
        return api_success(data)

    def delete(self, request, user_id):
        UserService().delete(request.user, user_id)
        return api_success({})


class UserBatchDeleteView(APIView):
    permission_classes = [HasDeleteUser]

    def post(self, request):
        serializer = BatchDeleteUsersSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = UserService().batch_delete(
            request.user,
            [str(item) for item in serializer.validated_data["user_ids"]],
        )
        return api_success(data)


class UserLogsView(APIView):
    permission_classes = [HasViewUser]

    def get(self, request, user_id):
        paging = PaginationService().parse(request.query_params)
        data = UserService().list_logs(
            user_id,
            page=paging["page"],
            page_size=paging["page_size"],
        )
        return api_success(data)
