from pyservicemaker import osd


class DetDrawer:
    def __init__(self, show_label=False, show_conf=False):
        self.show_label = show_label
        self.show_conf = show_conf

    def hide_osd_objects(self, frame_meta) -> None:
        for obj_meta in frame_meta.object_items:
            obj_meta.rect_params.border_width = 0
            obj_meta.text_params.display_text = b""
            obj_meta.text_params.set_bg_clr = 0

    def apply_rect_params(self, rect, data, box_color, box_width) -> None:
        r, g, b, a = box_color
        rect.left = data["left"]
        rect.top = data["top"]
        rect.width = data["width"]
        rect.height = data["height"]
        rect.border_width = int(box_width)
        rect.border_color = osd.Color(float(r), float(g), float(b), float(a))

    def build_display_text(self, item) -> str:
        parts = []
        if self.show_label and item["label"]:
            parts.append(item["label"])
        if self.show_conf:
            parts.append(f"{item['conf']:.2f}")
        display_text = " ".join(parts)
        return display_text

    def apply_label(self, obj_meta, item, text_color, text_bg_color) -> None:
        display_text = self.build_display_text(item)
        if display_text:
            rect = obj_meta.rect_params
            text = obj_meta.text_params
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
        self.apply_rect_params(obj_meta.rect_params, item["rect_params"], box_color, box_width)
        self.apply_label(obj_meta, item, text_color, text_bg_color)
        frame_meta.append(obj_meta)

    def __call__(
        self,
        batch_meta,
        result,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> None:
        frame_meta = next(iter(batch_meta.frame_items))
        self.hide_osd_objects(frame_meta)
        for item in result["objects"]:
            self.append_object(
                batch_meta,
                frame_meta,
                item,
                box_color,
                box_width,
                text_color,
                text_bg_color,
            )


class DetFadeDrawer(DetDrawer):
    def __init__(self, show_label=False, show_conf=False, interval=0, fade_time=0):
        super().__init__(show_label, show_conf)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.min_alpha = 0.2
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)
        self.frame_count = 0
        self.cache = []

    def fade_color(self, color, fade_alpha) -> tuple:
        r, g, b = color[:3]
        result = (float(r), float(g), float(b), float(fade_alpha))
        return result

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

    def __call__(
        self,
        batch_meta,
        result,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
    ) -> None:
        frame_meta = next(iter(batch_meta.frame_items))
        self.hide_osd_objects(frame_meta)
        self.frame_count += 1
        phase = (self.frame_count - 1) % self.runtime_interval
        if phase == 0:
            self.cache = result["objects"]
        fade_alpha = self.alpha_lut[phase]
        faded_box_color = self.fade_color(box_color, fade_alpha)
        faded_text_color = self.fade_color(text_color, fade_alpha)
        faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
        for item in self.cache:
            self.append_object(
                batch_meta,
                frame_meta,
                item,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
