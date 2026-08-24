from utils.tool.parser.det_parser import DetParser
from utils.tool.parser.result import new_result


class PresenceParser(DetParser):
    def __init__(self, interval=0):
        self.interval = int(interval)
        self.object_cache = {}

    def is_inference_frame(self, frame_meta) -> bool:
        inference = self.interval <= 0 or int(frame_meta.frame_number) % self.interval == 0
        return inference

    def collect(self, frame_meta) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        pad_index = result["pad_index"]
        inference = self.is_inference_frame(frame_meta)
        result["inference"] = inference
        for object_meta in frame_meta.object_items:
            result["objects"].append(self.parse_object(object_meta))
        if inference:
            self.object_cache[pad_index] = result["objects"]
        else:
            result["objects"] = self.object_cache.get(pad_index, [])
        result["num_objects"] = len(result["objects"])
        return result

    def process_frame(self, batch_meta, frame_meta) -> dict:
        result = self.collect(frame_meta)
        return result
