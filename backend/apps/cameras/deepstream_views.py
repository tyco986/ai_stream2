from asgiref.sync import async_to_sync
from rest_framework.permissions import IsAuthenticated
from rest_framework.views import APIView

from common.exceptions import DeepStreamUnavailableError
from common.response import success_response
from services.deepstream_client import deepstream_client


class DeepStreamHealthView(APIView):
    permission_classes = [IsAuthenticated]

    def get(self, request):
        try:
            resp = async_to_sync(deepstream_client.health_check)()
            return success_response(resp.json())
        except Exception:
            raise DeepStreamUnavailableError()


class DeepStreamStreamsView(APIView):
    permission_classes = [IsAuthenticated]

    def get(self, request):
        try:
            resp = async_to_sync(deepstream_client.get_streams)()
            return success_response(resp.json())
        except Exception:
            raise DeepStreamUnavailableError()
