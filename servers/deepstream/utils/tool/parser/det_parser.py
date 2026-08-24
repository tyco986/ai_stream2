from utils.tool.parser.result import new_result
from utils.tool.timer.decorator import timer

UNTRACKED_OBJECT_ID = (1 << 64) - 1


class DetParser:
    def parse_object_id(self, object_meta) -> int:
        return -1 if int(object_meta.object_id) == UNTRACKED_OBJECT_ID else int(object_meta.object_id)

    def parse_rect_params(self, rect) -> dict:
        rect_params = {
            "left": float(rect.left),
            "top": float(rect.top),
            "width": float(rect.width),
            "height": float(rect.height),
        }
        return rect_params

    def parse_object(self, object_meta) -> dict:
        rect = object_meta.rect_params
        label = str(object_meta.label) if object_meta.label else ""
        item = {
            "object": [
                int(round(float(rect.left))),
                int(round(float(rect.top))),
                int(round(float(rect.left) + float(rect.width))),
                int(round(float(rect.top) + float(rect.height))),
                round(float(object_meta.confidence), 2),
                int(object_meta.class_id),
                label,
                self.parse_object_id(object_meta),
            ],
            "rect_params": self.parse_rect_params(rect),
        }
        return item

    def fill_frame_meta(self, result, frame_meta) -> None:
        result["pad_index"] = int(frame_meta.pad_index)
        result["frame_number"] = int(frame_meta.frame_number)
        result["source_id"] = int(frame_meta.source_id)
        result["source_width"] = int(frame_meta.source_width)
        result["source_height"] = int(frame_meta.source_height)
        result["pipeline_width"] = int(frame_meta.pipeline_width)
        result["pipeline_height"] = int(frame_meta.pipeline_height)

    def process_frame(self, batch_meta, frame_meta) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        for object_meta in frame_meta.object_items:
            result["objects"].append(self.parse_object(object_meta))
        result["num_objects"] = len(result["objects"])
        return result

    @timer(result_key="parser")
    def __call__(self, batch_meta) -> list:
        results = [self.process_frame(batch_meta, frame_meta) for frame_meta in batch_meta.frame_items]
        return results
