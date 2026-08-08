from rest_framework.views import APIView

from pages.servers.permissions import HasChangeServer, HasViewServer
from pages.servers.services import (
    HealthProbeService,
    RestartService,
    ServerLogService,
    ServerStatusService,
)
from shared.http.exceptions import AppError
from shared.http.response import api_success


class ServerListView(APIView):
    permission_classes = [HasViewServer]

    def get(self, request):
        return api_success(ServerStatusService().list_servers())


class ServerRefreshView(APIView):
    permission_classes = [HasViewServer]

    def post(self, request):
        return api_success(ServerStatusService().refresh_all())


class ServerHealthView(APIView):
    permission_classes = [HasViewServer]

    def get(self, request, server_id):
        result = HealthProbeService().probe(server_id)
        if result["ok"]:
            data = result["body"] if isinstance(result["body"], dict) else {}
            return api_success(data)
        status_code = 504 if result["timed_out"] else 502
        raise AppError(result["detail"] or "upstream unreachable", status_code=status_code)


class ServerRestartView(APIView):
    permission_classes = [HasChangeServer]

    def post(self, request, server_id):
        return api_success(RestartService().restart(server_id))


class ServerLogsView(APIView):
    permission_classes = [HasViewServer]

    def get(self, request, server_id):
        raw_tail = request.query_params.get("tail", "500")
        try:
            tail = int(raw_tail)
        except (TypeError, ValueError) as exc:
            raise AppError("Invalid tail", status_code=400) from exc
        return api_success(ServerLogService().get_logs(server_id, tail=tail))
