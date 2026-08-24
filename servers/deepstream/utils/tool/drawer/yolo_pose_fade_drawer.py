from utils.tool.drawer.det_fade_drawer import DetFadeDrawer
from utils.tool.drawer.yolo_pose_drawer import YoloPoseDrawer
from utils.tool.parser.result import new_result


class YoloPoseFadeDrawer(DetFadeDrawer, YoloPoseDrawer):
    def __init__(
        self,
        show_label=False,
        show_conf=False,
        show_id=False,
        kpt_threshold=0.0,
        interval=0,
        fade_time=0,
    ):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id
        self.kpt_threshold = float(kpt_threshold)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.frame_width = 1
        self.frame_height = 1
        self.fade_alpha = 1.0
        self.min_alpha = 0.2
        self.shadow_box_color = (0.6, 0.0, 1.0, 1.0)
        self.frame_count = {}
        self.phase = {}
        self.object_cache = {}
        self.conf_cache = {}
        self.pose_cache = {}
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)
        self.init_osd_colors()

    def cache_item(self, object_meta) -> dict:
        return self.parse_object(object_meta, [])

    def pose_item(self, object_meta, frame_meta, object_index) -> dict:
        return self.parse_pose_object(object_meta)

    def osd_color(self, color):
        faded = self.fade_color(color, self.fade_alpha)
        return YoloPoseDrawer.osd_color(self, faded)

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
            self.draw_pose(batch_meta, frame_meta, item)
        return cache

    def item_rect(self, item) -> tuple[float, float, float, float]:
        rect = item["rect_params"]
        return (rect["left"], rect["top"], rect["width"], rect["height"])

    def transform_keypoints(self, keypoints, src_rect, dst_rect) -> list:
        src_left, src_top, src_width, src_height = src_rect
        dst_left, dst_top, dst_width, dst_height = dst_rect
        scale_x = dst_width / src_width if src_width else 1.0
        scale_y = dst_height / src_height if src_height else 1.0
        mapped = [
            [
                round(dst_left + (float(x) - src_left) * scale_x, 1),
                round(dst_top + (float(y) - src_top) * scale_y, 1),
                score,
            ]
            for x, y, score in keypoints
        ]
        return mapped

    def apply_pose_cache(self, pad_index, item) -> None:
        track_id = int(item["object"][7])
        cache = self.pose_cache.get(pad_index, {})
        keypoints = item["keypoints"]
        if keypoints and track_id >= 0:
            cache[track_id] = {
                "keypoints": keypoints,
                "rect": self.item_rect(item),
            }
            self.pose_cache[pad_index] = cache
        elif track_id >= 0 and track_id in cache:
            stored = cache[track_id]
            item["keypoints"] = self.transform_keypoints(
                stored["keypoints"],
                stored["rect"],
                self.item_rect(item),
            )

    def prune_pose_cache(self, pad_index, items) -> None:
        seen_ids = {int(item["object"][7]) for item in items if int(item["object"][7]) >= 0}
        cache = self.pose_cache.get(pad_index, {})
        self.pose_cache[pad_index] = {tid: cache[tid] for tid in seen_ids if tid in cache}

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
        self.frame_width = max(1, int(result["pipeline_width"]))
        self.frame_height = max(1, int(result["pipeline_height"]))
        pad_index = result["pad_index"]
        inference = self.is_inference_frame(frame_meta)
        result["inference"] = inference
        phase = 0 if inference else self.phase.get(pad_index, 0)
        fade_alpha = self.alpha_lut[phase]
        faded_box_color = self.fade_color(box_color, fade_alpha)
        faded_text_color = self.fade_color(text_color, fade_alpha)
        faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
        draw_box_color = faded_box_color
        draw_text_color = faded_text_color
        draw_text_bg_color = faded_text_bg_color
        self.fade_alpha = fade_alpha
        if not inference:
            draw_box_color = self.shadow_box_color
            draw_text_color = text_color
            draw_text_bg_color = text_bg_color
            self.fade_alpha = 1.0
        next_phase = (phase + 1) % self.runtime_interval
        old_cache = self.conf_cache.get(pad_index, {})
        new_cache = {}
        for object_index, object_meta in enumerate(frame_meta.object_items):
            item = self.pose_item(object_meta, frame_meta, object_index)
            self.apply_id_conf(old_cache, new_cache, object_meta, item)
            self.apply_pose_cache(pad_index, item)
            self.draw_inplace(
                object_meta,
                item,
                draw_box_color,
                box_width,
                draw_text_color,
                draw_text_bg_color,
            )
            self.draw_pose(batch_meta, frame_meta, item)
            result["objects"].append(item)
        if inference:
            self.conf_cache[pad_index] = new_cache
            next_phase = 1 % self.runtime_interval
        self.fade_alpha = fade_alpha
        fade_objects = []
        if not inference:
            fade_objects = self.draw_non_inference_rebuild(
                batch_meta,
                frame_meta,
                pad_index,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
        if not result["objects"]:
            result["objects"] = fade_objects
        if inference:
            self.object_cache[pad_index] = result["objects"]
        self.prune_pose_cache(pad_index, result["objects"])
        result["num_objects"] = len(result["objects"])
        self.frame_count[pad_index] = self.frame_count.get(pad_index, 0) + 1
        self.phase[pad_index] = next_phase
        return result
