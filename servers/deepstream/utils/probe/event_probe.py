from queue import Full, Queue

from utils.probe.utils.debouncer.presence_debouncer import PresenceDebouncer
from utils.probe.utils.parser.det_parser import DetBatchMetaParser

from .yolo_probe import DetRTSPProbe


class ObjectPresenceProbe(DetRTSPProbe):
    def __init__(self, debouncer=dict(), logger=dict(), messager=dict()):
        super().__init__(logger=logger, messager=messager)
        self.debouncer = PresenceDebouncer(**debouncer)
        self.queue_vis = None
        self.queue_raw = None

    def inject_queue(self, queue_vis: Queue, queue_raw: Queue):
        self.queue_vis = queue_vis
        self.queue_raw = queue_raw

    def handle_metadata(self, batch_meta):
        parser = DetBatchMetaParser(batch_meta)
        results = parser.results
        self.drawer(batch_meta, results)
        self.debouncer(results)
        self.logger(results)
        self.messager(results)

        assert self.queue_vis is not None, "Queue is not injected"
        assert self.queue_raw is not None, "Queue is not injected"
        self.enqueue_capture_tickets(results)

    def enqueue_capture_tickets(self, results: list[dict]):
        for frame_result in results:
            event = frame_result.get("event", {})
            if event.get("event_code") != 1:
                continue
            ticket = {
                "pad_index": int(frame_result["pad_index"]),
                "frame_number": int(frame_result["frame_number"]),
                "source_id": int(frame_result["source_id"]),
                "capture": True,
            }
            for capture_queue in (self.queue_raw, self.queue_vis):
                try:
                    capture_queue.put_nowait(dict(ticket))
                except Full:
                    pass
