class PoseKptsCache:
    def __init__(self):
        self.frames = {}

    def put_frame(self, source_id, frame_number, kpts_list):
        self.frames[(int(source_id), int(frame_number))] = kpts_list

    def pop_frame(self, source_id, frame_number):
        kpts_list = self.frames.pop((int(source_id), int(frame_number)), [])
        return kpts_list


class PoseBatchMetaParser:
    POSE_KEYPOINT_COUNT = 17

    def __init__(self, batch_meta, kpts_list=None):
        self.result = None
        self.batch_meta = batch_meta
        self.kpts_list = kpts_list
        self.get_result()

    def parse_rect_params(self, rect) -> dict:
        color = rect.border_color
        rect_params = {
            "left": float(rect.left),
            "top": float(rect.top),
            "width": float(rect.width),
            "height": float(rect.height),
            "border_width": int(rect.border_width),
            "border_color": {
                "r": float(color.r),
                "g": float(color.g),
                "b": float(color.b),
                "a": float(color.a),
            },
        }
        return rect_params

    @staticmethod
    def parse_kpts(mask) -> list[list[float]] | None:
        flat = [
            float(v)
            for v in mask.mask_array.ravel()[: PoseBatchMetaParser.POSE_KEYPOINT_COUNT * 3]
        ]
        kpts = None
        if flat:
            kpts = [
                [flat[i * 3], flat[i * 3 + 1], flat[i * 3 + 2]]
                for i in range(len(flat) // 3)
            ]
        return kpts

    def resolve_kpts(self, obj, index) -> list[list[float]] | None:
        kpts = None
        if self.kpts_list is not None:
            if index < len(self.kpts_list):
                kpts = self.kpts_list[index]
        else:
            kpts = PoseBatchMetaParser.parse_kpts(obj.mask_params)
        return kpts

    def parse_obj(self, obj, index) -> dict:
        rect = obj.rect_params
        result = {
            "box": [
                int(round(float(rect.left))),
                int(round(float(rect.top))),
                int(round(float(rect.left) + float(rect.width))),
                int(round(float(rect.top) + float(rect.height))),
            ],
            "conf": round(float(obj.confidence), 2),
            "cls": int(obj.class_id),
            "label": str(obj.label) if obj.label else "",
            "rect_params": self.parse_rect_params(rect),
            "kpts": self.resolve_kpts(obj, index),
        }
        return result

    def parse_frame(self, frame_meta) -> dict:
        objects = [
            self.parse_obj(obj, index)
            for index, obj in enumerate(frame_meta.object_items)
        ]
        result = {
            "pad_index": int(frame_meta.pad_index),
            "frame_number": int(frame_meta.frame_number),
            "source_id": int(frame_meta.source_id),
            "source_width": int(frame_meta.source_width),
            "source_height": int(frame_meta.source_height),
            "pipeline_width": int(frame_meta.pipeline_width),
            "pipeline_height": int(frame_meta.pipeline_height),
            "num_objects": len(objects),
            "objects": objects,
        }
        return result

    def get_result(self) -> dict:
        frame_meta = next(iter(self.batch_meta.frame_items))
        self.result = self.parse_frame(frame_meta)
        return self.result
