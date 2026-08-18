from utils.probe.utils.drawer.det_fade_drawer import DetFadeDrawer
from utils.probe.utils.drawer.pose2d_drawer import Pose2DDrawer
from utils.probe.utils.drawer.result import new_result


class Pose2DFadeDrawer(DetFadeDrawer, Pose2DDrawer):
    def __init__(
        self,
        show_label=False,
        show_conf=False,
        show_id=False,
        kpt_threshold=0.0,
        infer_height=256,
        infer_width=192,
        sgie_unique_id=2,
        interval=0,
        fade_time=0,
    ):
        self.show_label = show_label
        self.show_conf = show_conf
        self.show_id = show_id
        self.kpt_threshold = float(kpt_threshold)
        self.infer_height = int(infer_height)
        self.infer_width = int(infer_width)
        self.sgie_unique_id = int(sgie_unique_id)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.frame_width = 1
        self.frame_height = 1
        self.fade_alpha = 1.0
        self.min_alpha = 0.2
        self.frame_count = {}
        self.phase = {}
        self.object_cache = {}
        self.conf_cache = {}
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)

    def osd_color(self, color):
        faded = self.fade_color(color, self.fade_alpha)
        return Pose2DDrawer.osd_color(self, faded)

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
        self.fade_alpha = fade_alpha
        faded_box_color = self.fade_color(box_color, fade_alpha)
        faded_text_color = self.fade_color(text_color, fade_alpha)
        faded_text_bg_color = self.fade_color(text_bg_color, fade_alpha)
        next_phase = (phase + 1) % self.runtime_interval
        old_cache = self.conf_cache.get(pad_index, {})
        new_cache = {}
        source_id = int(frame_meta.source_id)
        frame_number = int(frame_meta.frame_number)
        for object_index, object_meta in enumerate(frame_meta.object_items):
            item = self.restore_object(object_meta, source_id, frame_number, object_index)
            self.apply_id_conf(old_cache, new_cache, object_meta, item)
            self.draw_inplace(
                object_meta,
                item,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
            self.draw_pose(batch_meta, frame_meta, item)
            result["objects"].append(item)
        if inference:
            self.conf_cache[pad_index] = new_cache
            self.object_cache[pad_index] = result["objects"]
            next_phase = 1 % self.runtime_interval
        elif not result["objects"]:
            result["objects"] = self.draw_non_inference_rebuild(
                batch_meta,
                frame_meta,
                pad_index,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
            )
        result["num_objects"] = len(result["objects"])
        self.frame_count[pad_index] = self.frame_count.get(pad_index, 0) + 1
        self.phase[pad_index] = next_phase
        return result
