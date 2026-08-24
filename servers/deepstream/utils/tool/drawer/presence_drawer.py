from utils.tool.debouncer.presence_debouncer import PresenceDebouncer
from utils.tool.drawer.det_fade_drawer import DetFadeDrawer
from utils.tool.parser.presence_parser import PresenceParser
from utils.tool.parser.result import new_result


class PresenceFadeDrawer(PresenceParser):
    EVENT_BOX_COLORS = {
        "0": (0.0, 1.0, 0.0, 1.0),
        "1": (1.0, 0.0, 0.0, 1.0),
        "2": (1.0, 1.0, 0.0, 1.0),
    }

    def __init__(self, drawer=dict(), debouncer=dict()):
        self.drawer = drawer
        self.debouncer = debouncer
        PresenceParser.__init__(self, interval=drawer.get("interval", 0))
        self.osd = DetFadeDrawer(**drawer)
        self.debouncer = PresenceDebouncer(**debouncer)
        self.result_cache = {}
        self.phase = {}

    def hide_object(self, object_meta) -> None:
        object_meta.rect_params.border_width = 0
        object_meta.rect_params.rotation_angle = 0.0
        object_meta.text_params.display_text = b""

    def resolve_box_color(self, event_code) -> tuple[float, float, float, float]:
        code = str(event_code)
        key = "0"
        if "1" in code:
            key = "1"
        elif "2" in code:
            key = "2"
        box_color = self.EVENT_BOX_COLORS[key]
        return box_color

    def collect(self, frame_meta) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        pad_index = result["pad_index"]
        inference = self.is_inference_frame(frame_meta)
        result["inference"] = inference
        for object_meta in frame_meta.object_items:
            item = self.parse_object(object_meta)
            self.hide_object(object_meta)
            result["objects"].append(item)
        if inference:
            self.object_cache[pad_index] = result["objects"]
            self.phase[pad_index] = 0
        else:
            result["objects"] = self.object_cache.get(pad_index, [])
        result["num_objects"] = len(result["objects"])
        return result

    def rebuild(self, batch_meta, frame_meta, result) -> None:
        pad_index = int(frame_meta.pad_index)
        phase = self.phase.get(pad_index, 0)
        fade_alpha = self.osd.alpha_lut[phase]
        box_color = self.resolve_box_color(result["event"]["event_codes"])
        faded_box_color = self.osd.fade_color(box_color, fade_alpha)
        faded_text_color = self.osd.fade_color((1.0, 1.0, 1.0, 1.0), fade_alpha)
        faded_text_bg_color = self.osd.fade_color((0.0, 0.0, 0.0, 0.6), fade_alpha)
        for item in result["objects"]:
            self.osd.append_object(
                batch_meta,
                frame_meta,
                item,
                faded_box_color,
                2,
                faded_text_color,
                faded_text_bg_color,
            )
        self.osd.frame_count[pad_index] = self.osd.frame_count.get(pad_index, 0) + 1
        self.phase[pad_index] = (phase + 1) % self.osd.runtime_interval

    def process_frame(self, batch_meta, frame_meta) -> dict:
        result = self.collect(frame_meta)
        if result["inference"]:
            self.debouncer(result)
            self.result_cache[result["pad_index"]] = result
        else:
            cached = self.result_cache.get(result["pad_index"])
            if cached is not None:
                event = dict(cached["event"])
                event["capture"] = False
                result["event"] = event
            else:
                self.debouncer(result)
                self.result_cache[result["pad_index"]] = result
        self.rebuild(batch_meta, frame_meta, result)
        return result

    def __call__(self, batch_meta) -> list:
        results = [self.process_frame(batch_meta, frame_meta) for frame_meta in batch_meta.frame_items]
        return results
