"""Unit tests for MessageHandler (no DeepStream container required)."""

import sys
from pathlib import Path
from unittest.mock import MagicMock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


class TestMessageHandler:

    def _make_handler(self, monkeypatch):
        import types

        source_map = {}
        engine_monitor = MagicMock(started=False)

        mock_psm = types.ModuleType("pyservicemaker")
        mock_psm.DynamicSourceMessage = type("DynamicSourceMessage", (), {})
        mock_psm.StateTransitionMessage = type("StateTransitionMessage", (), {})
        mock_psm.PipelineState = types.SimpleNamespace(PLAYING="PLAYING")
        mock_psm.utils = types.SimpleNamespace()
        monkeypatch.setitem(sys.modules, "pyservicemaker", mock_psm)
        monkeypatch.setitem(sys.modules, "pipeline.builder", types.SimpleNamespace(PipelineBuilder=object))
        monkeypatch.setitem(sys.modules, "daemons.command_consumer", types.SimpleNamespace(CommandConsumer=object))

        if "main" in sys.modules:
            monkeypatch.delitem(sys.modules, "main")

        from main import MessageHandler

        handler = MessageHandler(source_map, engine_monitor)
        return handler, source_map, mock_psm

    def test_source_add_updates_map(self, monkeypatch):
        handler, source_map, mock_psm = self._make_handler(monkeypatch)
        message = mock_psm.DynamicSourceMessage()
        message.source_added = True
        message.source_id = 0
        message.sensor_id = "cam_001"
        message.sensor_name = "Camera 1"
        message.uri = "rtsp://example/cam_001"

        handler(message)

        assert source_map == {"cam_001": 0}

    def test_source_remove_clears_map(self, monkeypatch):
        handler, source_map, mock_psm = self._make_handler(monkeypatch)
        source_map["cam_001"] = 0

        message = mock_psm.DynamicSourceMessage()
        message.source_added = False
        message.source_id = 0
        message.sensor_id = "cam_001"

        handler(message)

        assert source_map == {}
