from rest_framework.permissions import AllowAny
from rest_framework.views import APIView

from pages.login.serializers import LoginSerializer
from pages.login.services import LoginService
from shared.http.response import api_success


class LoginView(APIView):
    authentication_classes = []
    permission_classes = [AllowAny]

    def post(self, request):
        serializer = LoginSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        data = LoginService().authenticate_and_login(
            request,
            serializer.validated_data["username"],
            serializer.validated_data["password"],
            serializer.validated_data.get("new_password"),
        )
        return api_success(data)
