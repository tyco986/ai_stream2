from pyservicemaker import osd

from utils.tool.parser.det_parser import UNTRACKED_OBJECT_ID, DetParser
from utils.tool.parser.result import new_result
from utils.tool.timer.decorator import timer

# frame_meta.object_items is single-pass; parse and draw in one loop.
# Do not list(object_items) then mutate — held refs segfault.


class DetDrawer(DetParser):
    def __init__(self, show_label=False, show_conf=False, show_id=False):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id
        self.init_osd_colors()

    def init_osd_colors(self) -> None:
        self.box_osd_color = osd.Color(0.0, 1.0, 0.0, 1.0)
        self.text_osd_color = osd.Color(1.0, 1.0, 1.0, 1.0)
        self.text_bg_osd_color = osd.Color(0.0, 0.0, 0.0, 0.6)
        self.kpt_osd_color = osd.Color(1.0, 0.5, 0.0, 1.0)

    def fill_osd_color(self, osd_color, color) -> osd.Color:
        r, g, b, a = color
        osd_color.r = float(r)
        osd_color.g = float(g)
        osd_color.b = float(b)
        osd_color.a = float(a)
        return osd_color

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
            text.font_params.color = self.fill_osd_color(self.text_osd_color, text_color)
            text.set_bg_clr = 1
            text.text_bg_clr = self.fill_osd_color(self.text_bg_osd_color, text_bg_color)

    def apply_box_color(self, object_meta, box_color, box_width) -> None:
        object_meta.rect_params.border_color = self.fill_osd_color(
            self.box_osd_color, box_color
        )
        object_meta.rect_params.border_width = int(box_width)

    def apply_rect_params(self, rect, data, box_color, box_width) -> None:
        rect.left = data["left"]
        rect.top = data["top"]
        rect.width = data["width"]
        rect.height = data["height"]
        rect.border_width = int(box_width)
        rect.border_color = self.fill_osd_color(self.box_osd_color, box_color)
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

    @timer(result_key="drawer")
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
