import cv2
import numpy as np
from PIL import Image

from utils.tool.receiver.utils.det_labelme_exporter import DetLabelmeExporter


class SegLabelmeExporter(DetLabelmeExporter):
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

    def labelme_polygon_shape(self, obj, points) -> dict:
        shape = {
            "label": str(obj["label"]),
            "points": points,
            "group_id": None,
            "shape_type": "polygon",
            "flags": {},
        }
        return shape

    def labelme_shapes(self, objects) -> list:
        shapes = []
        for obj in objects:
            polygons = self.mask_polygons(obj)
            if polygons:
                shapes.extend(
                    [self.labelme_polygon_shape(obj, points) for points in polygons]
                )
            else:
                shapes.append(self.labelme_shape(obj))
        return shapes
