from pathlib import Path


class DetYoloExporter:
    def __init__(self, yolo_dir):
        self.yolo_dir = Path(yolo_dir)

    def yolo_line(self, obj, image_width, image_height) -> str:
        left = float(obj["left"])
        top = float(obj["top"])
        width = float(obj["width"])
        height = float(obj["height"])
        cx = (left + width / 2.0) / image_width
        cy = (top + height / 2.0) / image_height
        bw = width / image_width
        bh = height / image_height
        line = f"{int(obj['class_id'])} {cx:.6f} {cy:.6f} {bw:.6f} {bh:.6f}"
        return line

    def yolo_lines(self, objects, image_width, image_height) -> list:
        lines = []
        if image_width > 0 and image_height > 0:
            lines = [self.yolo_line(obj, image_width, image_height) for obj in objects]
        return lines

    def __call__(self, objects, image_width, image_height, stem, image_name) -> None:
        self.yolo_dir.mkdir(parents=True, exist_ok=True)
        lines = self.yolo_lines(objects, image_width, image_height)
        path = self.yolo_dir / f"{stem}.txt"
        path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
