from rest_framework.permissions import IsAuthenticated
from rest_framework.views import APIView
from rest_framework_simplejwt.views import TokenObtainPairView, TokenRefreshView

from common.response import success_response
from common.throttles import LoginRateThrottle

from .serializers import UserSerializer


class LoginView(TokenObtainPairView):
    throttle_classes = [LoginRateThrottle]


class RefreshView(TokenRefreshView):
    pass


class MeView(APIView):
    permission_classes = [IsAuthenticated]

    def get(self, request):
        serializer = UserSerializer(request.user)
        return success_response(serializer.data)
