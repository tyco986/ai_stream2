from channels.generic.websocket import AsyncJsonWebsocketConsumer


class DetectionConsumer(AsyncJsonWebsocketConsumer):
    """检测结果实时推送 — group: detections_{organization_id}"""

    async def connect(self):
        user = self.scope.get("user")
        if not user or not user.organization_id:
            await self.close(code=4001)
            return
        self.group_name = f"detections_{user.organization_id}"
        await self.channel_layer.group_add(self.group_name, self.channel_name)
        await self.accept()

    async def disconnect(self, close_code):
        if hasattr(self, "group_name"):
            await self.channel_layer.group_discard(self.group_name, self.channel_name)

    async def detection_new(self, event):
        await self.send_json({"type": "detection.new", "data": event["data"]})


class CameraStatusConsumer(AsyncJsonWebsocketConsumer):
    """摄像头状态变更推送 — group: camera_status_{organization_id}"""

    async def connect(self):
        user = self.scope.get("user")
        if not user or not user.organization_id:
            await self.close(code=4001)
            return
        self.group_name = f"camera_status_{user.organization_id}"
        await self.channel_layer.group_add(self.group_name, self.channel_name)
        await self.accept()

    async def disconnect(self, close_code):
        if hasattr(self, "group_name"):
            await self.channel_layer.group_discard(self.group_name, self.channel_name)

    async def camera_status(self, event):
        await self.send_json({"type": "camera.status", "data": event["data"]})


class AlertConsumer(AsyncJsonWebsocketConsumer):
    """报警事件推送 — group: alerts_{organization_id}"""

    async def connect(self):
        user = self.scope.get("user")
        if not user or not user.organization_id:
            await self.close(code=4001)
            return
        self.group_name = f"alerts_{user.organization_id}"
        await self.channel_layer.group_add(self.group_name, self.channel_name)
        await self.accept()

    async def disconnect(self, close_code):
        if hasattr(self, "group_name"):
            await self.channel_layer.group_discard(self.group_name, self.channel_name)

    async def alert_new(self, event):
        await self.send_json({"type": "alert.new", "data": event["data"]})
