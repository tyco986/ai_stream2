"""DeepStream proxy endpoint tests — health, streams, auth."""
from unittest.mock import AsyncMock, MagicMock, patch

import pytest


def _mock_ds_response(json_data):
    """Regular MagicMock because async_to_sync unwraps the coroutine,
    so the returned object is used synchronously (resp.json())."""
    resp = MagicMock()
    resp.json.return_value = json_data
    return resp


@pytest.mark.django_db
class TestDeepStreamProxyHealth:
    @patch("apps.cameras.deepstream_views.deepstream_client")
    def test_health_check_success(self, mock_ds, operator_client):
        mock_ds.health_check = AsyncMock(
            return_value=_mock_ds_response({"status": "ready"}),
        )
        resp = operator_client.get("/api/v1/deepstream/health/")
        assert resp.status_code == 200
        assert resp.data["data"]["status"] == "ready"

    @patch("apps.cameras.deepstream_views.deepstream_client")
    def test_health_check_unavailable(self, mock_ds, operator_client):
        mock_ds.health_check = AsyncMock(side_effect=Exception("connection refused"))
        resp = operator_client.get("/api/v1/deepstream/health/")
        assert resp.status_code == 503
        assert resp.data["code"] == "DEEPSTREAM_UNAVAILABLE"

    def test_health_check_unauthenticated(self, api_client):
        resp = api_client.get("/api/v1/deepstream/health/")
        assert resp.status_code == 401


@pytest.mark.django_db
class TestDeepStreamProxyStreams:
    @patch("apps.cameras.deepstream_views.deepstream_client")
    def test_list_streams_success(self, mock_ds, operator_client):
        mock_ds.get_streams = AsyncMock(
            return_value=_mock_ds_response({
                "stream-info": {"stream-info": [
                    {"id": "cam-001", "status": "playing"},
                ]},
            }),
        )
        resp = operator_client.get("/api/v1/deepstream/streams/")
        assert resp.status_code == 200
        streams = resp.data["data"]["stream-info"]["stream-info"]
        assert len(streams) == 1
        assert streams[0]["id"] == "cam-001"

    @patch("apps.cameras.deepstream_views.deepstream_client")
    def test_list_streams_unavailable(self, mock_ds, operator_client):
        mock_ds.get_streams = AsyncMock(side_effect=Exception("timeout"))
        resp = operator_client.get("/api/v1/deepstream/streams/")
        assert resp.status_code == 503

    def test_list_streams_unauthenticated(self, api_client):
        resp = api_client.get("/api/v1/deepstream/streams/")
        assert resp.status_code == 401

    @patch("apps.cameras.deepstream_views.deepstream_client")
    def test_viewer_can_access_streams(self, mock_ds, viewer_client):
        mock_ds.get_streams = AsyncMock(
            return_value=_mock_ds_response({"stream-info": {"stream-info": []}}),
        )
        resp = viewer_client.get("/api/v1/deepstream/streams/")
        assert resp.status_code == 200
