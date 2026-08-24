import numpy as np

from utils.tool.parser.det_parser import DetParser
from utils.tool.parser.result import new_result


class YoloPoseParser(DetParser):
    def decode_keypoints(self, object_meta, left, top, width, height) -> list:
        arr = np.asarray(object_meta.mask_params.mask_array, dtype=np.float32).reshape(-1)
        keypoints = []
        if arr.size >= 3 and arr.size % 3 == 0:
            keypoints = [
                [
                    round(left + float(arr[i * 3]) * width, 1),
                    round(top + float(arr[i * 3 + 1]) * height, 1),
                    round(float(arr[i * 3 + 2]), 3),
                ]
                for i in range(int(arr.size) // 3)
            ]
        return keypoints

    def parse_object(self, object_meta, keypoints) -> dict:
        item = DetParser.parse_object(self, object_meta)
        item["keypoints"] = keypoints
        return item

    def parse_pose_object(self, object_meta) -> dict:
        rect = object_meta.rect_params
        keypoints = self.decode_keypoints(
            object_meta,
            float(rect.left),
            float(rect.top),
            float(rect.width),
            float(rect.height),
        )
        item = self.parse_object(object_meta, keypoints)
        return item

    def process_frame(self, batch_meta, frame_meta) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        for object_meta in frame_meta.object_items:
            result["objects"].append(self.parse_pose_object(object_meta))
        result["num_objects"] = len(result["objects"])
        return result
