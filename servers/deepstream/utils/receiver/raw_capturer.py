from pathlib import Path

import cupy
import cv2
from pyservicemaker import BufferRetriever

from utils.bridge.presence_capture_state import PresenceCaptureState


class RawCapturer(BufferRetriever):
    def __init__(self, capturer=dict()):
        super().__init__()
        self.capturer = capturer
        self.output_dir = Path(self.capturer.get("output_dir", "/root/output"))
        self.raw_dir = self.output_dir / "images"

    def consume(self, buffer):
        frame_meta = next(iter(buffer.batch_meta.frame_items))
        pad_index = int(frame_meta.pad_index)
        frame_number = int(frame_meta.frame_number)
        result = PresenceCaptureState.get(pad_index, frame_number)
        if result and result.get("event", {}).get("capture"):
            capture_id = int(result["event"]["capture_id"])
            output_path = self.raw_dir / f"raw_{pad_index:03d}_{capture_id:08d}.png"
            tensor = buffer.extract(int(frame_meta.batch_id)).clone()
            frame = cupy.from_dlpack(tensor)[:, :, :3]
            if frame.dtype != cupy.uint8:
                frame = cupy.clip(frame, 0, 255).astype(cupy.uint8)
            frame_cpu = cupy.asnumpy(frame.copy())
            cupy.cuda.Device().synchronize()
            output_path.parent.mkdir(parents=True, exist_ok=True)
            cv2.imwrite(str(output_path), cv2.cvtColor(frame_cpu, cv2.COLOR_RGB2BGR))
        return 1
