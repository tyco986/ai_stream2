import cv2
import numpy as np
from PIL import Image

from utils.receiver.utils.det_yolo_exporter import DetYoloExporter


class SegYoloExporter(DetYoloExporter):
    def mask_polygons(self, obj) -> list:
        polygons = []
        mask = obj.get("mask")
        if mask is not None:
            box_width = max(1, int(obj["width"]))
            box_height = max(1, int(obj["height"]))
            binary = np.asarray(
                Image.fromarray(mask, mode="L").resize(
                    (box_width, box_height), Image.Resampling.NEAREST
                )
            )
            contours, _ = cv2.findContours(
                binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
            )
            left = float(obj["left"])
            top = float(obj["top"])
            polygons = [
                [[float(point[0][0] + left), float(point[0][1] + top)] for point in contour]
                for contour in contours
                if len(contour) >= 3
            ]
        return polygons

    def yolo_polygon_line(self, class_id, points, image_width, image_height) -> str:
        coords = [
            value
            for point in points
            for value in (f"{point[0] / image_width:.6f}", f"{point[1] / image_height:.6f}")
        ]
        line = f"{int(class_id)} " + " ".join(coords)
        return line

    def yolo_lines(self, objects, image_width, image_height) -> list:
        lines = []
        if image_width > 0 and image_height > 0:
            for obj in objects:
                polygons = self.mask_polygons(obj)
                if polygons:
                    lines.extend(
                        [
                            self.yolo_polygon_line(
                                obj["class_id"], points, image_width, image_height
                            )
                            for points in polygons
                        ]
                    )
                else:
                    lines.append(super().yolo_line(obj, image_width, image_height))
        return lines
