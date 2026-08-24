from pathlib import Path

import cupy
import cv2
import numpy as np
from pyservicemaker import BufferRetriever

from utils.tool.bridge.presence_capture_state import PresenceCaptureState
from utils.tool.receiver.utils.det_labelme_exporter import DetLabelmeExporter
from utils.tool.receiver.utils.det_yolo_exporter import DetYoloExporter
from utils.tool.receiver.utils.seg_labelme_exporter import SegLabelmeExporter
from utils.tool.receiver.utils.seg_yolo_exporter import SegYoloExporter

YOLO_EXPORTERS = {
    "det": DetYoloExporter,
    "seg": SegYoloExporter,
}
LABELME_EXPORTERS = {
    "det": DetLabelmeExporter,
    "seg": SegLabelmeExporter,
}


class VisCapturer(BufferRetriever):
    def __init__(self, capturer=dict()):
        super().__init__()
        self.capturer = capturer
        self.output_dir = Path(self.capturer.get("output_dir", "/root/output"))
        self.labelme_dir = self.output_dir / "labelme"
        self.vis_dir = self.output_dir / "vis"
        self.yolo_dir = self.output_dir / "labels"
        self.mask_color = self.capturer.get("mask_color", (0, 255, 0, 128))
        task = self.capturer.get("label_task", "det")
        self.label_exporters = [
            cls(directory)
            for cls, directory in (
                (YOLO_EXPORTERS.get(task), self.yolo_dir),
                (LABELME_EXPORTERS.get(task), self.labelme_dir),
            )
            if cls is not None
        ]

    def parse_objects(self, frame_meta) -> list:
        objects = []
        for object_meta in frame_meta.object_items:
            rect = object_meta.rect_params
            label = str(object_meta.label) if object_meta.label else str(int(object_meta.class_id))
            item = {
                "left": float(rect.left),
                "top": float(rect.top),
                "width": float(rect.width),
                "height": float(rect.height),
                "class_id": int(object_meta.class_id),
                "label": label,
                "mask": None,
            }
            # Mask binding only exposes mask_array/threshold (no size/width/height).
            mask = object_meta.mask_params
            mask_array = list(mask.mask_array)
            width = max(1, int(rect.width))
            height = max(1, int(rect.height))
            if len(mask_array) == width * height:
                bitmap = np.asarray(mask_array, dtype=np.float32).reshape(height, width)
                item["mask"] = (bitmap >= float(mask.threshold)).astype(np.uint8) * 255
            objects.append(item)
        return objects

    def draw_masks(self, image, objects) -> np.ndarray:
        result = image.copy()
        r, g, b, a = self.mask_color
        alpha = float(a) / 255.0
        image_height, image_width = result.shape[:2]
        for obj in objects:
            mask = obj["mask"]
            if mask is not None:
                left = int(obj["left"])
                top = int(obj["top"])
                box_width = max(1, int(obj["width"]))
                box_height = max(1, int(obj["height"]))
                mask_resized = cv2.resize(
                    mask, (box_width, box_height), interpolation=cv2.INTER_NEAREST
                )
                x1 = max(0, left)
                y1 = max(0, top)
                x2 = min(image_width, left + box_width)
                y2 = min(image_height, top + box_height)
                if x2 > x1 and y2 > y1:
                    mask_crop = mask_resized[y1 - top : y2 - top, x1 - left : x2 - left]
                    weight = (mask_crop.astype(np.float32) / 255.0) * alpha
                    weight = weight[:, :, None]
                    roi = result[y1:y2, x1:x2].astype(np.float32)
                    color = np.array([r, g, b], dtype=np.float32)
                    blended = roi * (1.0 - weight) + color * weight
                    result[y1:y2, x1:x2] = blended.astype(np.uint8)
        return result

    def consume(self, buffer):
        frame_meta = next(iter(buffer.batch_meta.frame_items))
        pad_index = int(frame_meta.pad_index)
        frame_number = int(frame_meta.frame_number)
        result = PresenceCaptureState.get(pad_index, frame_number)
        if result and result.get("event", {}).get("capture"):
            capture_id = int(result["event"]["capture_id"])
            raw_stem = f"raw_{pad_index:03d}_{capture_id:08d}"
            raw_image_name = f"{raw_stem}.png"
            output_path = self.vis_dir / f"vis_{pad_index:03d}_{capture_id:08d}.jpg"
            tensor = buffer.extract(int(frame_meta.batch_id)).clone()
            frame = cupy.from_dlpack(tensor)[:, :, :3]
            if frame.dtype != cupy.uint8:
                frame = cupy.clip(frame, 0, 255).astype(cupy.uint8)
            frame_cpu = cupy.asnumpy(frame.copy())
            cupy.cuda.Device().synchronize()
            objects = self.parse_objects(frame_meta)
            image = self.draw_masks(frame_cpu, objects)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            cv2.imwrite(str(output_path), cv2.cvtColor(image, cv2.COLOR_RGB2BGR))
            image_height, image_width = image.shape[:2]
            image_path = f"../images/{raw_image_name}"
            for exporter in self.label_exporters:
                exporter(
                    objects,
                    image_width,
                    image_height,
                    raw_stem,
                    image_path,
                )
        return 1
