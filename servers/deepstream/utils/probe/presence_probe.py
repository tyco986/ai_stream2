from utils.probe.utils.debouncer.presence_debouncer import PresenceDebouncer
from utils.probe.utils.parser.det_parser import DetBatchMetaParser

from .det_probe import DetRTSPProbe

PRESENCE_META_TYPE = 8193


class PresenceProbe(DetRTSPProbe):
    def __init__(self, debouncer=dict(), logger=dict(), messager=dict()):
        super().__init__(logger=logger, messager=messager)
        self.debouncer = PresenceDebouncer(**debouncer)

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        result = parser.result
        self.drawer(batch_meta, result)
        self.debouncer(result)
        self.logger(result)
        self.messager(result)
        self.stamp_event_meta(batch_meta, result)

    def stamp_event_meta(self, batch_meta, result: dict) -> None:
        event = result.get("event", {"event_code": "", "window": []})
        frame_meta = next(iter(batch_meta.frame_items))
        event_msg = batch_meta.acquire_event_message_meta()
        payload = {"event": event}
        event_msg.set_user_data_json(payload, PRESENCE_META_TYPE)
        frame_meta.append(event_msg)
