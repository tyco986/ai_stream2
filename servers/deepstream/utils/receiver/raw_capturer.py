from pathlib import Path

import cupy
from PIL import Image
from pyservicemaker import BufferRetriever

PRESENCE_META_TYPE = 8193


class RawCapturer(BufferRetriever):
    def __init__(self, capturer=dict()):
        super().__init__()
        self.capturer = capturer
        self.output_dir = Path(self.capturer.get("output_dir", "/root/output"))

    def consume(self, buffer):
        frame_meta = next(iter(buffer.batch_meta.frame_items))
        item = next(iter(frame_meta.user_meta_items(PRESENCE_META_TYPE)))
        data = item.get_user_data_json()
        event = data.get("event", {"event_code": ""})
        if "1" in event["event_code"]:
            pad_index = int(frame_meta.pad_index)
            frame_number = int(frame_meta.frame_number)
            output_path = self.output_dir / f"raw_{pad_index}_{frame_number}.png"
            tensor = buffer.extract(int(frame_meta.batch_id)).clone()
            frame = cupy.from_dlpack(tensor)[:, :, :3]
            if frame.dtype != cupy.uint8:
                frame = cupy.clip(frame, 0, 255).astype(cupy.uint8)
            image = Image.fromarray(cupy.asnumpy(frame))
            output_path.parent.mkdir(parents=True, exist_ok=True)
            image.save(output_path, format="PNG")
        return 1
