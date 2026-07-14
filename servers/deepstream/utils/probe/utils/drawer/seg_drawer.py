import numpy as np
from pyservicemaker import osd


class SegDrawer:
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

    def parse_mask_shape(self, bitmap) -> tuple[int, int]:
        shape = bitmap.shape
        if len(shape) == 2:
            mask_height, mask_width = int(shape[0]), int(shape[1])
        else:
            flat_size = int(bitmap.size)
            side = int(flat_size**0.5)
            mask_width = side if side * side == flat_size else flat_size
            mask_height = flat_size // mask_width if mask_width else 0
        return mask_width, mask_height

    def parse_mask_params(self, mask) -> dict | None:
        bitmap = np.asarray(mask.mask_array, dtype=np.float32).copy()
        mask_params = None
        if bitmap.size > 0:
            mask_width, mask_height = self.parse_mask_shape(bitmap)
            mask_params = {
                "mask_data": bitmap,
                "mask_width": mask_width,
                "mask_height": mask_height,
                "mask_size": int(bitmap.size),
                "mask_threshold": float(mask.threshold),
            }
        return mask_params

    def parse_object(self, object_meta) -> dict:
        rect = object_meta.rect_params
        result = {
            "box": [
                int(round(float(rect.left))),
                int(round(float(rect.top))),
                int(round(float(rect.left) + float(rect.width))),
                int(round(float(rect.top) + float(rect.height))),
            ],
            "conf": round(float(object_meta.confidence), 2),
            "cls": int(object_meta.class_id),
            "label": str(object_meta.label) if object_meta.label else "",
            "rect_params": self.parse_rect_params(rect),
            "mask_params": self.parse_mask_params(object_meta.mask_params),
        }
        return result

    def build_display_text(self, item) -> str:
        parts = []
        if self.show_label and item["label"]:
            parts.append(item["label"])
        if self.show_conf:
            parts.append(f"{item['conf']:.2f}")
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

    def apply_mask_color(self, object_meta, mask_color, box_width) -> None:
        r, g, b, a = mask_color
        object_meta.rect_params.border_color = osd.Color(float(r), float(g), float(b), float(a))
        object_meta.rect_params.border_width = int(box_width)

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

    def __call__(
        self,
        batch_meta,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        frame_meta = next(iter(batch_meta.frame_items))
        objects = []
        for object_meta in frame_meta.object_items:
            item = self.parse_object(object_meta)
            self.apply_mask_color(object_meta, mask_color, box_width)
            self.apply_label(object_meta, item, text_color, text_bg_color)
            objects.append(item)
        result = self.get_result(frame_meta, objects)
        return result


class SegFadeDrawer(SegDrawer):
    def __init__(self, show_label=False, show_conf=False, interval=0, fade_time=0):
        self.show_label = show_label
        self.show_conf = show_conf
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.min_alpha = 0.2
        self.frame_count = 0
        self.phase = 0
        self.object_cache = []
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)

    def build_alpha_lut(self, interval, fade_time) -> list[float]:
        result = [1.0]
        if fade_time > 0:
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

    def __call__(
        self,
        batch_meta,
        mask_color=(0.0, 1.0, 0.0, 0.5),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        frame_meta = next(iter(batch_meta.frame_items))
        if self.phase == 0:
            self.object_cache = []
        fade_alpha = self.alpha_lut[self.phase]
        faded_mask_color = self.fade_color(mask_color, fade_alpha)
        faded_text_color = self.fade_color(text_color, fade_alpha)
        faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
        objects = []
        for index, object_meta in enumerate(frame_meta.object_items):
            item = self.parse_object(object_meta)
            cached_item = item
            if self.phase == 0:
                self.object_cache.append(item)
            else:
                cached_item = (
                    self.object_cache[index]
                    if index < len(self.object_cache)
                    else item
                )
            self.apply_mask_color(object_meta, faded_mask_color, box_width)
            self.apply_label(object_meta, cached_item, faded_text_color, faded_text_bg_color)
            objects.append(item)
        result = self.get_result(frame_meta, objects)
        self.frame_count += 1
        self.phase = self.frame_count % self.runtime_interval
        return result
