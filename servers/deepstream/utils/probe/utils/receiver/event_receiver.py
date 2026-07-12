from pathlib import Path
from queue import Empty, Queue

import cupy
from PIL import Image
from pyservicemaker import BufferRetriever


class FrameEncoder:
    def __init__(self, kind):
        self.kind = kind
        self.image_format = "PNG" if kind == "raw" else "JPEG"
        self.file_suffix = "png" if kind == "raw" else "jpg"

    def save(self, buffer, batch_id, output_path):
        tensor = buffer.extract(batch_id).clone()
        frame = cupy.from_dlpack(tensor)[:, :, :3]
        if frame.dtype != cupy.uint8:
            frame = cupy.clip(frame, 0, 255).astype(cupy.uint8)
        image = Image.fromarray(cupy.asnumpy(frame))
        output_path.parent.mkdir(parents=True, exist_ok=True)
        image.save(output_path, format=self.image_format)


class EventReceiver(BufferRetriever):
    def __init__(self, stream_index: int, kind: str, output_dir):
        super().__init__()
        self.capture_queue = None
        self.stream_index = stream_index
        self.output_dir = Path(output_dir)
        self.encoder = FrameEncoder(kind)
        self.pending = set()

    def inject_queue(self, capture_queue: Queue):
        self.capture_queue = capture_queue

    def sync_pending_tickets(self):
        assert self.capture_queue is not None, "Queue is not injected"
        while True:
            ticket = None
            try:
                ticket = self.capture_queue.get_nowait()
            except Empty:
                break
            pad_index = int(ticket["pad_index"])
            frame_number = int(ticket["frame_number"])
            if pad_index != self.stream_index:
                continue
            if not ticket.get("capture"):
                continue
            self.pending.add((pad_index, frame_number))

    def build_output_path(self, pad_index, frame_number):
        suffix = self.encoder.file_suffix
        filename = f"{self.encoder.kind}_{pad_index}_{frame_number}.{suffix}"
        return self.output_dir / filename

    def consume(self, buffer):
        self.sync_pending_tickets()
        for frame_meta in buffer.batch_meta.frame_items:
            pad_index = int(frame_meta.pad_index)
            frame_number = int(frame_meta.frame_number)
            if pad_index != self.stream_index:
                continue
            key = (pad_index, frame_number)
            if key not in self.pending:
                continue
            self.pending.discard(key)
            output_path = self.build_output_path(pad_index, frame_number)
            self.encoder.save(buffer, int(frame_meta.batch_id), output_path)
        return 1
