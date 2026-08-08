import json
from pathlib import Path


class DetLabelmeExporter:
    def __init__(self, labelme_dir):
        self.labelme_dir = Path(labelme_dir)

    def labelme_shape(self, obj) -> dict:
        left = float(obj["left"])
        top = float(obj["top"])
        right = left + float(obj["width"])
        bottom = top + float(obj["height"])
        shape = {
            "label": str(obj["label"]),
            "points": [[left, top], [right, bottom]],
            "group_id": None,
            "shape_type": "rectangle",
            "flags": {},
        }
        return shape

    def labelme_shapes(self, objects) -> list:
        shapes = [self.labelme_shape(obj) for obj in objects]
        return shapes

    def __call__(self, objects, image_width, image_height, stem, image_name) -> None:
        self.labelme_dir.mkdir(parents=True, exist_ok=True)
        payload = {
            "version": "5.0.1",
            "flags": {},
            "shapes": self.labelme_shapes(objects),
            "imagePath": image_name,
            "imageData": None,
            "imageHeight": int(image_height),
            "imageWidth": int(image_width),
        }
        path = self.labelme_dir / f"{stem}.json"
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
