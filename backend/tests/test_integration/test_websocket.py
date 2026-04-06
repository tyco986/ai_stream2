"""WebSocket consumer tests — auth, groups, message delivery, tenant isolation."""
import asyncio

import pytest
from channels.db import database_sync_to_async
from channels.testing import WebsocketCommunicator
from rest_framework_simplejwt.tokens import RefreshToken

from config.asgi import application
from tests.factories import OrganizationFactory, UserFactory


@database_sync_to_async
def _create_user_and_token(org=None):
    if org is None:
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


async def _safe_disconnect(communicator):
    """Disconnect ignoring CancelledError from already-closed transports."""
    try:
        await communicator.disconnect()
    except asyncio.CancelledError:
        pass


@pytest.mark.django_db(transaction=True)
@pytest.mark.asyncio
class TestWebSocketTenantIsolation:
    async def test_org_a_does_not_receive_org_b_detections(self):
        """Messages sent to org_b's group must not reach org_a's consumer."""
        user_a, token_a = await _create_user_and_token()
        org_b = await database_sync_to_async(OrganizationFactory)(name="Isolation Org B")
        user_b, token_b = await _create_user_and_token(org=org_b)

        comm_a = _make_communicator("ws/detections/", token=token_a)
        comm_b = _make_communicator("ws/detections/", token=token_b)

        connected_a, _ = await comm_a.connect()
        connected_b, _ = await comm_b.connect()
        assert connected_a and connected_b

        from channels.layers import get_channel_layer
        channel_layer = get_channel_layer()

        org_b_id = str(user_b.organization_id)
        await channel_layer.group_send(
            f"detections_{org_b_id}",
            {
                "type": "detection.new",
                "data": [{"camera_uid": "cam-b-secret", "object_count": 99}],
            },
        )

        msg_b = await comm_b.receive_json_from(timeout=3)
        assert msg_b["data"][0]["camera_uid"] == "cam-b-secret"

        with pytest.raises(asyncio.TimeoutError):
            await comm_a.receive_json_from(timeout=1)

        await _safe_disconnect(comm_b)
        await _safe_disconnect(comm_a)

    async def test_org_a_does_not_receive_org_b_alerts(self):
        user_a, token_a = await _create_user_and_token()
        org_b = await database_sync_to_async(OrganizationFactory)(name="Alert Org B")
        user_b, token_b = await _create_user_and_token(org=org_b)

        comm_a = _make_communicator("ws/alerts/", token=token_a)
        comm_b = _make_communicator("ws/alerts/", token=token_b)

        connected_a, _ = await comm_a.connect()
        connected_b, _ = await comm_b.connect()
        assert connected_a and connected_b

        from channels.layers import get_channel_layer
        channel_layer = get_channel_layer()

        org_b_id = str(user_b.organization_id)
        await channel_layer.group_send(
            f"alerts_{org_b_id}",
            {
                "type": "alert.new",
                "data": {"alert_id": "secret-alert", "rule_name": "intrusion"},
            },
        )

        msg_b = await comm_b.receive_json_from(timeout=3)
        assert msg_b["data"]["alert_id"] == "secret-alert"

        with pytest.raises(asyncio.TimeoutError):
            await comm_a.receive_json_from(timeout=1)

        await _safe_disconnect(comm_b)
        await _safe_disconnect(comm_a)
