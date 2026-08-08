from pyservicemaker import osd

from utils.probe.utils.drawer.result import new_result

# frame_meta.object_items is single-pass; parse and draw in one loop.
# Do not list(object_items) then mutate — held refs segfault.

UNTRACKED_OBJECT_ID = (1 << 64) - 1


class DetDrawer:
    def __init__(self, show_label=False, show_conf=False, show_id=False):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id

    def parse_object_id(self, object_meta) -> int:
        return -1 if int(object_meta.object_id) == UNTRACKED_OBJECT_ID else int(object_meta.object_id)

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

    def build_display_text(self, item) -> str:
        obj = item["object"]
        parts = []
        if self.show_label and obj[6]:
            parts.append(obj[6])
        if self.show_conf:
            parts.append(f"{obj[4]:.2f}")
        if self.show_id:
            parts.append(f"{obj[7]}")
        display_text = "|".join(parts)
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

    def fill_frame_meta(self, result, frame_meta) -> None:
        result["pad_index"] = int(frame_meta.pad_index)
        result["frame_number"] = int(frame_meta.frame_number)
        result["source_id"] = int(frame_meta.source_id)
        result["source_width"] = int(frame_meta.source_width)
        result["source_height"] = int(frame_meta.source_height)
        result["pipeline_width"] = int(frame_meta.pipeline_width)
        result["pipeline_height"] = int(frame_meta.pipeline_height)

    def process_frame(
        self,
        batch_meta,
        frame_meta,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> dict:
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
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
            result["objects"].append(item)
        result["num_objects"] = len(result["objects"])
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
