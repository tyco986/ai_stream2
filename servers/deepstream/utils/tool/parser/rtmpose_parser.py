import cupy

from utils.tool.parser.result import new_result
from utils.tool.parser.yolo_pose_parser import YoloPoseParser
from utils.tool.preprocessor.rect_expander import RectExpander

KEYPOINTS = "keypoints"
PADDING = 1.25


class RtmposeParser(YoloPoseParser):
    def __init__(
        self,
        infer_height=256,
        infer_width=192,
        sgie_unique_id=2,
    ):
        self.infer_height = int(infer_height)
        self.infer_width = int(infer_width)
        self.sgie_unique_id = int(sgie_unique_id)

    def round_up_2(self, value) -> int:
        return (int(value) + 1) & ~1

    def round_down_2(self, value) -> int:
        return int(value) & ~1

    def layer_array(self, layers, name):
        array = cupy.from_dlpack(layers[name].clone())
        while array.ndim > 2 and array.shape[0] == 1:
            array = array[0]
        return array

    def object_layers(self, object_meta):
        layers = None
        for user_meta in object_meta.tensor_items:
            tensor_meta = user_meta.as_tensor_output()
            if int(tensor_meta.unique_id) == self.sgie_unique_id:
                layers = tensor_meta.get_layers()
        return layers

    def map_to_frame(self, kpts, left, top, width, height) -> list:
        kpts = cupy.asnumpy(kpts)
        src_left = self.round_up_2(left)
        src_top = self.round_up_2(top)
        src_width = max(2, self.round_down_2(width))
        src_height = max(2, self.round_down_2(height))
        fit_height = self.infer_width * src_height / float(src_width)
        dest_width = self.infer_width
        dest_height = int(fit_height)
        if fit_height > self.infer_height:
            dest_width = int(self.infer_height * src_width / float(src_height))
            dest_height = self.infer_height
        offset_left = (self.infer_width - dest_width) // 2
        offset_top = (self.infer_height - dest_height) // 2
        ratio_x = dest_width / float(src_width)
        ratio_y = dest_height / float(src_height)
        keypoints = [
            [
                round(src_left + (float(kpts[index, 0]) - offset_left) / ratio_x, 1),
                round(src_top + (float(kpts[index, 1]) - offset_top) / ratio_y, 1),
                round(float(kpts[index, 2]), 3),
            ]
            for index in range(kpts.shape[0])
        ]
        return keypoints

    def decode_keypoints(self, object_meta, left, top, width, height) -> list:
        layers = self.object_layers(object_meta)
        keypoints = []
        if layers is not None:
            keypoints = self.map_to_frame(
                self.layer_array(layers, KEYPOINTS),
                left,
                top,
                width,
                height,
            )
        return keypoints

    def restore_object(self, object_meta, source_id, frame_number, object_index) -> dict:
        rect = object_meta.rect_params
        keypoints = self.decode_keypoints(
            object_meta,
            float(rect.left),
            float(rect.top),
            float(rect.width),
            float(rect.height),
        )
        RectExpander.restore(rect, source_id, frame_number, object_index)
        item = self.parse_object(object_meta, keypoints)
        return item

    def process_frame(self, batch_meta, frame_meta) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        source_id = int(frame_meta.source_id)
        frame_number = int(frame_meta.frame_number)
        for object_index, object_meta in enumerate(frame_meta.object_items):
            result["objects"].append(
                self.restore_object(object_meta, source_id, frame_number, object_index)
            )
        result["num_objects"] = len(result["objects"])
        return result
