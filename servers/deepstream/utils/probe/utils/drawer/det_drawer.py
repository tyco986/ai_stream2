from pyservicemaker import osd


class DetDrawer:
    def apply_rect_params(self, rect, data, box_color, box_width) -> None:
        r, g, b, a = box_color
        rect.left = data["left"]
        rect.top = data["top"]
        rect.width = data["width"]
        rect.height = data["height"]
        rect.border_width = int(box_width)
        rect.border_color = osd.Color(float(r), float(g), float(b), float(a))

    def apply_label(self, obj_meta, item, text_color, text_bg_color, label) -> None:
        display_label = label if label else item["label"]
        if display_label:
            rect = obj_meta.rect_params
            text = obj_meta.text_params
            text.display_text = display_label.encode("utf-8")
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
        label,
    ) -> None:
        obj_meta = batch_meta.acquire_object_meta()
        self.apply_rect_params(obj_meta.rect_params, item["rect_params"], box_color, box_width)
        self.apply_label(obj_meta, item, text_color, text_bg_color, label)
        frame_meta.append(obj_meta)

    def __call__(
        self,
        batch_meta,
        results,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
        label="",
    ) -> None:
        frame_meta_by_pad = {
            int(frame_meta.pad_index): frame_meta
            for frame_meta in batch_meta.frame_items
        }
        for frame_result in results:
            frame_meta = frame_meta_by_pad[int(frame_result["pad_index"])]
            for item in frame_result["objects"]:
                self.append_object(
                    batch_meta,
                    frame_meta,
                    item,
                    box_color,
                    box_width,
                    text_color,
                    text_bg_color,
                    label,
                )


class DetFadeDrawer(DetDrawer):
    def __init__(self, interval=0, fade_time=0):
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.min_alpha = 0.2
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)
        self.frame_counts = {}
        self.caches = {}

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
        results,
        box_color=(0.0, 1.0, 0.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
        label="",
    ) -> None:
        frame_meta_by_pad = {
            int(frame_meta.pad_index): frame_meta
            for frame_meta in batch_meta.frame_items
        }
        for frame_result in results:
            pad_index = int(frame_result["pad_index"])
            count = self.frame_counts.get(pad_index, 0) + 1
            self.frame_counts[pad_index] = count
            phase = (count - 1) % self.runtime_interval
            if phase == 0:
                self.caches[pad_index] = frame_result["objects"]
            cache = self.caches.get(pad_index, [])
            fade_alpha = self.alpha_lut[phase]
            faded_box_color = self.fade_color(box_color, fade_alpha)
            faded_text_color = self.fade_color(text_color, fade_alpha)
            faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
            frame_meta = frame_meta_by_pad[pad_index]
            for item in cache:
                self.append_object(
                    batch_meta,
                    frame_meta,
                    item,
                    faded_box_color,
                    box_width,
                    faded_text_color,
                    faded_text_bg_color,
                    label,
                )
