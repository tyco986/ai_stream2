from utils.probe.utils.drawer.det_drawer import UNTRACKED_OBJECT_ID, DetDrawer
from utils.probe.utils.drawer.result import new_result


class DetFadeDrawer(DetDrawer):
    def __init__(
        self,
        show_label=False,
        show_conf=False,
        show_id=False,
        interval=0,
        fade_time=0,
    ):
        super().__init__(show_label, show_conf, show_id)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.min_alpha = 0.2
        self.frame_count = {}
        self.phase = {}
        self.object_cache = {}
        self.conf_cache = {}
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)

    def is_inference_frame(self, frame_meta) -> bool:
        inference = (
            self.interval <= 0
            or int(frame_meta.frame_number) % (self.interval + 1) == 0
        )
        return inference

    def apply_id_conf(self, old_cache, new_cache, object_meta, item) -> None:
        obj = item["object"]
        conf = float(obj[4])
        track_id = int(obj[7])
        if conf >= 0:
            if track_id >= 0:
                new_cache[track_id] = conf
        elif track_id >= 0 and track_id in old_cache:
            conf = old_cache[track_id]
            obj[4] = conf
            object_meta.confidence = conf
            new_cache[track_id] = conf

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
        obj_meta.object_id = UNTRACKED_OBJECT_ID if int(obj[7]) < 0 else int(obj[7])
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
        result = new_result()
        self.fill_frame_meta(result, frame_meta)
        pad_index = result["pad_index"]
        inference = self.is_inference_frame(frame_meta)
        result["inference"] = inference
        phase = 0 if inference else self.phase.get(pad_index, 0)
        fade_alpha = self.alpha_lut[phase]
        faded_box_color = self.fade_color(box_color, fade_alpha)
        faded_text_color = self.fade_color(text_color, fade_alpha)
        faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
        if inference:
            old_cache = self.conf_cache.get(pad_index, {})
            new_cache = {}
            for object_meta in frame_meta.object_items:
                item = self.parse_object(object_meta)
                self.apply_id_conf(old_cache, new_cache, object_meta, item)
                self.draw_inplace(
                    object_meta,
                    item,
                    faded_box_color,
                    box_width,
                    faded_text_color,
                    faded_text_bg_color,
                )
                result["objects"].append(item)
            self.conf_cache[pad_index] = new_cache
            self.object_cache[pad_index] = result["objects"]
            next_phase = 1 % self.runtime_interval
        else:
            result["objects"] = self.draw_non_inference_rebuild(
                batch_meta,
                frame_meta,
                pad_index,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
            next_phase = (phase + 1) % self.runtime_interval
        result["num_objects"] = len(result["objects"])
        self.frame_count[pad_index] = self.frame_count.get(pad_index, 0) + 1
        self.phase[pad_index] = next_phase
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
