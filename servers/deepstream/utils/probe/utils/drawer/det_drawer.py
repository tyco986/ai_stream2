from pyservicemaker import osd

# frame_meta.object_items is single-pass; parse and draw in one loop.
# Do not list(object_items) then mutate — held refs segfault.


class DetDrawer:
    def __init__(self, show_label=False, show_conf=False):
        self.show_label = show_label
        self.show_conf = show_conf

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

    def parse_object(self, object_meta) -> dict:
        rect = object_meta.rect_params
        label = str(object_meta.label) if object_meta.label else ""
        result = {
            "object": [
                int(round(float(rect.left))),
                int(round(float(rect.top))),
                int(round(float(rect.left) + float(rect.width))),
                int(round(float(rect.top) + float(rect.height))),
                round(float(object_meta.confidence), 2),
                int(object_meta.class_id),
                label,
            ],
            "rect_params": self.parse_rect_params(rect),
        }
        return result

    def build_display_text(self, item) -> str:
        obj = item["object"]
        parts = []
        if self.show_label and obj[6]:
            parts.append(obj[6])
        if self.show_conf:
            parts.append(f"{obj[4]:.2f}")
        display_text = " ".join(parts)
        return display_text

    def apply_label(self, object_meta, item, text_color, text_bg_color) -> None:
        display_text = self.build_display_text(item)
        if display_text:
            rect = object_meta.rect_params
            text = object_meta.text_params
            text.display_text = display_text.encode("utf-8")
            text.x_offset = int(rect.left)
            text.y_offset = max(0, int(rect.top) - 14)
            text.font_params.name = osd.FontFamily.Serif
            text.font_params.size = 12
            r, g, b, a = text_color
            text.font_params.color = osd.Color(float(r), float(g), float(b), float(a))
            text.set_bg_clr = 1
            r, g, b, a = text_bg_color
            text.text_bg_clr = osd.Color(float(r), float(g), float(b), float(a))

    def apply_box_color(self, object_meta, box_color, box_width) -> None:
        r, g, b, a = box_color
        object_meta.rect_params.border_color = osd.Color(float(r), float(g), float(b), float(a))
        object_meta.rect_params.border_width = int(box_width)

    def apply_rect_params(self, rect, data, box_color, box_width) -> None:
        r, g, b, a = box_color
        rect.left = data["left"]
        rect.top = data["top"]
        rect.width = data["width"]
        rect.height = data["height"]
        rect.border_width = int(box_width)
        rect.border_color = osd.Color(float(r), float(g), float(b), float(a))
        rect.rotation_angle = 0.0

    def draw_inplace(
        self,
        object_meta,
        item,
        box_color,
        box_width,
        text_color,
        text_bg_color,
    ) -> None:
        object_meta.rect_params.rotation_angle = 0.0
        self.apply_box_color(object_meta, box_color, box_width)
        self.apply_label(object_meta, item, text_color, text_bg_color)

    def get_result(self, frame_meta, objects) -> dict:
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

    def process_frame(
        self,
        batch_meta,
        frame_meta,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        objects = []
        for object_meta in frame_meta.object_items:
            item = self.parse_object(object_meta)
            self.draw_inplace(
                object_meta,
                item,
                box_color,
                box_width,
                text_color,
                text_bg_color,
            )
            objects.append(item)
        result = self.get_result(frame_meta, objects)
        return result

    def __call__(
        self,
        batch_meta,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> list:
        results = []
        for frame_meta in batch_meta.frame_items:
            results.append(
                self.process_frame(
                    batch_meta,
                    frame_meta,
                    box_color=box_color,
                    box_width=box_width,
                    text_color=text_color,
                    text_bg_color=text_bg_color,
                )
            )
        return results


class DetFadeDrawer(DetDrawer):
    def __init__(self, show_label=False, show_conf=False, interval=0, fade_time=0):
        super().__init__(show_label, show_conf)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.min_alpha = 0.2
        self.frame_count = {}
        self.phase = {}
        self.object_cache = {}
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)

    def append_object(
        self,
        batch_meta,
        frame_meta,
        item,
        box_color,
        box_width,
        text_color,
        text_bg_color,
    ) -> None:
        obj_meta = batch_meta.acquire_object_meta()
        obj = item["object"]
        obj_meta.class_id = int(obj[5])
        obj_meta.confidence = float(obj[4])
        obj_meta.label = obj[6]
        self.apply_rect_params(obj_meta.rect_params, item["rect_params"], box_color, box_width)
        self.apply_label(obj_meta, item, text_color, text_bg_color)
        frame_meta.append(obj_meta)

    def draw_non_inference_rebuild(
        self,
        batch_meta,
        frame_meta,
        pad_index,
        box_color,
        box_width,
        text_color,
        text_bg_color,
    ) -> list:
        cache = self.object_cache.get(pad_index, [])
        for item in cache:
            self.append_object(
                batch_meta,
                frame_meta,
                item,
                box_color,
                box_width,
                text_color,
                text_bg_color,
            )
        return cache

    def build_alpha_lut(self, interval, fade_time) -> list[float]:
        if fade_time <= 0:
            result = [1.0]
        else:
            mid = interval // 2
            tail = interval - mid
            triangle = []
            for i in range(interval + 1):
                if i <= mid:
                    t = 1.0 - i / mid if mid > 0 else 1.0
                else:
                    t = (i - mid) / tail if tail > 0 else 0.0
                alpha = self.min_alpha + (1.0 - self.min_alpha) * t
                triangle.append(round(alpha, 2))
            result = []
            for index in range(fade_time):
                result.extend(triangle if index == 0 else triangle[1:])
        return result

    def fade_color(self, color, fade_alpha) -> tuple[float, float, float, float]:
        r, g, b = color[:3]
        result = (float(r), float(g), float(b), float(fade_alpha))
        return result

    def process_frame(
        self,
        batch_meta,
        frame_meta,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        pad_index = int(frame_meta.pad_index)
        phase = self.phase.get(pad_index, 0)
        fade_alpha = self.alpha_lut[phase]
        faded_box_color = self.fade_color(box_color, fade_alpha)
        faded_text_color = self.fade_color(text_color, fade_alpha)
        faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
        objects = []
        for object_meta in frame_meta.object_items:
            item = self.parse_object(object_meta)
            self.draw_inplace(
                object_meta,
                item,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
            objects.append(item)
        if objects:
            self.object_cache[pad_index] = objects
        else:
            objects = self.draw_non_inference_rebuild(
                batch_meta,
                frame_meta,
                pad_index,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
        result = self.get_result(frame_meta, objects)
        frame_count = self.frame_count.get(pad_index, 0) + 1
        self.frame_count[pad_index] = frame_count
        self.phase[pad_index] = frame_count % self.runtime_interval
        return result

    def __call__(
        self,
        batch_meta,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> list:
        results = []
        for frame_meta in batch_meta.frame_items:
            results.append(
                self.process_frame(
                    batch_meta,
                    frame_meta,
                    box_color=box_color,
                    box_width=box_width,
                    text_color=text_color,
                    text_bg_color=text_bg_color,
                )
            )
        return results
