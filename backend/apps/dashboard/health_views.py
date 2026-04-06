from django.conf import settings
from django.core.cache import cache
from django.db import connection
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework.views import APIView


class LivenessView(APIView):
    permission_classes = [AllowAny]
    throttle_classes = []
    authentication_classes = []

    def get(self, request):
        return Response({"status": "alive"})


class ReadinessView(APIView):
    permission_classes = [AllowAny]
    throttle_classes = []
    authentication_classes = []

    def get(self, request):
        checks = {
            "database": self._check_db(),
            "redis": self._check_redis(),
        }
        if settings.HEALTH_CHECK_KAFKA:
            checks["kafka"] = self._check_kafka()

        healthy = all(checks.values())
        return Response(
            {"status": "healthy" if healthy else "degraded", "checks": checks},
            status=200 if healthy else 503,
        )

    def _check_db(self):
        try:
            connection.ensure_connection()
            return True
        except Exception:
            return False

    def _check_redis(self):
        try:
            cache.set("_health_check", "ok", 10)
            return cache.get("_health_check") == "ok"
        except Exception:
            return False

    def _check_kafka(self):
        try:
            from confluent_kafka import Producer
            p = Producer({"bootstrap.servers": settings.KAFKA_BOOTSTRAP_SERVERS})
            p.flush(timeout=5)
            return True
        except Exception:
            return False
