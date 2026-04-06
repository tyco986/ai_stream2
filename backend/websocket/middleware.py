from channels.db import database_sync_to_async
from rest_framework_simplejwt.exceptions import TokenError
from rest_framework_simplejwt.tokens import AccessToken

from apps.accounts.models import User


class JWTAuthMiddleware:
    """WebSocket JWT 认证中间件 — 从 query parameter 提取 token。

    浏览器 WebSocket API 不支持自定义 HTTP Header，
    JWT 通过 ws://host/ws/xxx/?token=<access_token> 传递。
    """

    def __init__(self, app):
        self.app = app

    async def __call__(self, scope, receive, send):
        query_string = scope.get("query_string", b"").decode()
        params = dict(
            pair.split("=", 1) for pair in query_string.split("&") if "=" in pair
        )
        token_str = params.get("token")
        if not token_str:
            await send({"type": "websocket.close", "code": 4001})
            return

        user = await self._get_user(token_str)
        if user is None:
            await send({"type": "websocket.close", "code": 4001})
            return

        scope["user"] = user
        await self.app(scope, receive, send)

    @database_sync_to_async
    def _get_user(self, token_str):
        try:
            token = AccessToken(token_str)
            return User.objects.select_related("organization").get(id=token["user_id"])
        except (TokenError, User.DoesNotExist, KeyError, ValueError):
            return None
