"""WebSocket consumer tests — auth, groups, message delivery."""
import pytest
from channels.db import database_sync_to_async
from channels.testing import WebsocketCommunicator
from rest_framework_simplejwt.tokens import RefreshToken

from config.asgi import application
from tests.factories import OrganizationFactory, UserFactory


@database_sync_to_async
def _create_user_and_token():
    org = OrganizationFactory()
    user = UserFactory(organization=org, role="operator")
    token = str(RefreshToken.for_user(user).access_token)
    return user, token


def _make_communicator(path, token=None):
    url = path
    if token:
        url = f"{path}?token={token}"
    return WebsocketCommunicator(application, url)


@pytest.mark.django_db(transaction=True)
@pytest.mark.asyncio
class TestWebSocketAuth:
    async def test_no_token_rejected(self):
        communicator = _make_communicator("ws/detections/")
        connected, code = await communicator.connect()
        assert not connected or code == 4001
        await communicator.disconnect()

    async def test_invalid_token_rejected(self):
        communicator = _make_communicator("ws/detections/", token="invalid.jwt.token")
        connected, code = await communicator.connect()
        assert not connected or code == 4001
        await communicator.disconnect()

    async def test_valid_token_accepted(self):
        user, token = await _create_user_and_token()
        communicator = _make_communicator("ws/detections/", token=token)
        connected, _ = await communicator.connect()
        assert connected
        await communicator.disconnect()


@pytest.mark.django_db(transaction=True)
@pytest.mark.asyncio
class TestWebSocketMessaging:
    async def test_detection_consumer_receives_group_send(self):
        user, token = await _create_user_and_token()
        communicator = _make_communicator("ws/detections/", token=token)
        connected, _ = await communicator.connect()
        assert connected

        from channels.layers import get_channel_layer
        channel_layer = get_channel_layer()

        org_id = str(user.organization_id)
        await channel_layer.group_send(
            f"detections_{org_id}",
            {
                "type": "detection.new",
                "data": [{"camera_uid": "cam-001", "object_count": 3}],
            },
        )

        msg = await communicator.receive_json_from(timeout=3)
        assert msg["type"] == "detection.new"
        assert msg["data"][0]["camera_uid"] == "cam-001"
        await communicator.disconnect()

    async def test_alert_consumer_receives_group_send(self):
        user, token = await _create_user_and_token()
        communicator = _make_communicator("ws/alerts/", token=token)
        connected, _ = await communicator.connect()
        assert connected

        from channels.layers import get_channel_layer
        channel_layer = get_channel_layer()

        org_id = str(user.organization_id)
        await channel_layer.group_send(
            f"alerts_{org_id}",
            {
                "type": "alert.new",
                "data": {"alert_id": "test-123", "rule_name": "intrusion"},
            },
        )

        msg = await communicator.receive_json_from(timeout=3)
        assert msg["type"] == "alert.new"
        assert msg["data"]["alert_id"] == "test-123"
        await communicator.disconnect()
