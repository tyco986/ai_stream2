import httpx
import structlog
from django.conf import settings
from structlog.contextvars import get_contextvars

logger = structlog.get_logger(__name__)


class _MockResponse:
    """Fake httpx.Response for development without DeepStream."""

    def __init__(self, status_code=200, json_data=None):
        self.status_code = status_code
        self._json = json_data or {}

    def json(self):
        return self._json

    def raise_for_status(self):
        pass


class DeepStreamClient:
    """长连接池代理 DeepStream REST API。

    httpx.AsyncClient 内部维护连接池（默认 100 连接），
    复用 TCP 连接避免每次请求的握手开销。禁止每次请求新建 AsyncClient。
    """

    def __init__(self):
        self._mock = getattr(settings, "DEEPSTREAM_MOCK", False)
        if not self._mock:
            self._client = httpx.AsyncClient(
                base_url=settings.DEEPSTREAM_REST_URL,
                timeout=httpx.Timeout(connect=5.0, read=10.0, write=5.0, pool=5.0),
            )
        else:
            self._client = None
            logger.info("DeepStreamClient running in MOCK mode")

    def _trace_headers(self):
        """Propagate X-Request-ID to outbound calls for cross-service tracing."""
        ctx = get_contextvars()
        request_id = ctx.get("request_id")
        if request_id:
            return {"X-Request-ID": request_id}
        return {}

    async def add_stream(self, camera_id, camera_name, rtsp_url):
        if self._mock:
            logger.info("MOCK add_stream", camera_id=camera_id)
            return _MockResponse(200, {"status": "ok"})
        return await self._client.post("/api/v1/stream/add", headers=self._trace_headers(), json={
            "key": "sensor",
            "value": {
                "camera_id": camera_id,
                "camera_name": camera_name,
                "camera_url": rtsp_url,
                "change": "camera_add",
            },
        })

    async def remove_stream(self, camera_id, rtsp_url):
        if self._mock:
            logger.info("MOCK remove_stream", camera_id=camera_id)
            return _MockResponse(200, {"status": "ok"})
        return await self._client.post("/api/v1/stream/remove", headers=self._trace_headers(), json={
            "key": "sensor",
            "value": {
                "camera_id": camera_id,
                "camera_url": rtsp_url,
                "change": "camera_remove",
            },
        })

    async def get_streams(self):
        if self._mock:
            return _MockResponse(200, {"stream-info": {"stream-info": []}})
        return await self._client.get("/api/v1/stream/get-stream-info", headers=self._trace_headers())

    async def get_stream_info(self):
        if self._mock:
            return _MockResponse(200, {"stream-info": {"stream-info": []}})
        return await self._client.get("/api/v1/stream/get-stream-info", headers=self._trace_headers())

    async def health_check(self):
        if self._mock:
            return _MockResponse(200, {"status": "ready"})
        return await self._client.get("/api/v1/health/get-dsready-state", headers=self._trace_headers())

    async def close(self):
        if self._client:
            await self._client.aclose()


deepstream_client = DeepStreamClient()
