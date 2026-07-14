from pyservicemaker import osd

from utils.probe.utils.drawer.det_drawer import DetDrawer, DetFadeDrawer


class PoseDrawer(DetDrawer):
    MAX_ELEMENTS_IN_DISPLAY_META = 16
    POSE_KEYPOINT_COUNT = 17
    skeleton = [
        [16, 14], [14, 12], [17, 15], [15, 13], [12, 13], [6, 12], [7, 13], [6, 7],
        [6, 8], [7, 9], [8, 10], [9, 11], [2, 3], [1, 2], [1, 3], [2, 4], [3, 5],
        [4, 6], [5, 7],
    ]

    def __init__(self, infer_width, infer_height, show_label=False, show_conf=False):
        super().__init__(show_label, show_conf)
        self.infer_width = int(infer_width)
        self.infer_height = int(infer_height)

    def flatten_kpts(self, item) -> list[float]:
        kpts = item.get("kpts")
        flat = []
        if kpts:
            flat = [float(v) for kpt in kpts for v in kpt[:3]]
        return flat

    def pose_frame_transform(self, pipeline_width, pipeline_height) -> tuple[float, float, float]:
        frame_w = pipeline_width
        frame_h = pipeline_height
        gain = min(self.infer_width / frame_w, self.infer_height / frame_h)
        pad_x = (self.infer_width - frame_w * gain) * 0.5
        pad_y = (self.infer_height - frame_h * gain) * 0.5
        return gain, pad_x, pad_y

    def next_display_meta(self, batch_meta, display_meta, element_count, display_metas):
        meta = display_meta
        if meta is None or element_count >= self.MAX_ELEMENTS_IN_DISPLAY_META:
            meta = batch_meta.acquire_display_meta()
        if meta not in display_metas:
            display_metas.append(meta)
        return meta

    def append_keypoints(
        self,
        batch_meta,
        frame_meta,
        item,
        kpt_color,
        kpt_bg_color,
        kpt_line_color,
        kpt_radius,
        kpt_line_width,
    ) -> None:
        flat = self.flatten_kpts(item)
        if not flat:
            return

        frame_w = int(frame_meta.pipeline_width)
        frame_h = int(frame_meta.pipeline_height)
        gain, pad_x, pad_y = self.pose_frame_transform(frame_w, frame_h)
        display_meta = None
        display_metas = []
        element_count = 0
        r, g, b, a = kpt_color
        point_color = osd.Color(float(r), float(g), float(b), float(a))
        r, g, b, a = kpt_bg_color
        point_bg_color = osd.Color(float(r), float(g), float(b), float(a))
        r, g, b, a = kpt_line_color
        line_color = osd.Color(float(r), float(g), float(b), float(a))

        for joint_idx in range(len(flat) // 3):
            base = joint_idx * 3
            xc = (flat[base + 0] - pad_x) / gain
            yc = (flat[base + 1] - pad_y) / gain
            display_meta = self.next_display_meta(
                batch_meta, display_meta, element_count, display_metas
            )
            element_count += 1
            circle = osd.Circle()
            circle.xc = int(min(frame_w - 1, max(0, xc)))
            circle.yc = int(min(frame_h - 1, max(0, yc)))
            circle.radius = int(kpt_radius)
            circle.color = point_color
            circle.has_bg_color = 1
            circle.bg_color = point_bg_color
            display_meta.add_circle(circle)

        for joint_a, joint_b in self.skeleton:
            idx_a = (joint_a - 1) * 3
            idx_b = (joint_b - 1) * 3
            if idx_a + 2 >= len(flat) or idx_b + 2 >= len(flat):
                continue
            x1 = (flat[idx_a + 0] - pad_x) / gain
            y1 = (flat[idx_a + 1] - pad_y) / gain
            x2 = (flat[idx_b + 0] - pad_x) / gain
            y2 = (flat[idx_b + 1] - pad_y) / gain
            display_meta = self.next_display_meta(
                batch_meta, display_meta, element_count, display_metas
            )
            element_count += 1
            line = osd.Line()
            line.x1 = int(min(frame_w - 1, max(0, x1)))
            line.y1 = int(min(frame_h - 1, max(0, y1)))
            line.x2 = int(min(frame_w - 1, max(0, x2)))
            line.y2 = int(min(frame_h - 1, max(0, y2)))
            line.width = int(kpt_line_width)
            line.color = line_color
            display_meta.add_line(line)

        [frame_meta.append(meta) for meta in display_metas]

    def append_object(
        self,
        batch_meta,
        frame_meta,
        item,
        box_color,
        box_width,
        text_color,
        text_bg_color,
        kpt_color,
        kpt_bg_color,
        kpt_line_color,
        kpt_radius,
        kpt_line_width,
    ) -> None:
        obj_meta = batch_meta.acquire_object_meta()
        self.apply_rect_params(obj_meta.rect_params, item["rect_params"], box_color, box_width)
        self.apply_label(obj_meta, item, text_color, text_bg_color)
        frame_meta.append(obj_meta)
        self.append_keypoints(
            batch_meta,
            frame_meta,
            item,
            kpt_color,
            kpt_bg_color,
            kpt_line_color,
            kpt_radius,
            kpt_line_width,
        )

    def __call__(
        self,
        batch_meta,
        result,
        box_color=(0.0, 0.0, 1.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
        kpt_color=(1.0, 1.0, 1.0, 1.0),
        kpt_bg_color=(0.0, 0.0, 1.0, 1.0),
        kpt_radius=6,
        kpt_line_color=(0.0, 0.0, 1.0, 1.0),
        kpt_line_width=2,
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
                kpt_color,
                kpt_bg_color,
                kpt_line_color,
                kpt_radius,
                kpt_line_width,
            )


class PoseFadeDrawer(PoseDrawer, DetFadeDrawer):
    def __init__(self, infer_width, infer_height, show_label=False, show_conf=False, interval=0, fade_time=0):
        DetDrawer.__init__(self, show_label, show_conf)
        self.infer_width = int(infer_width)
        self.infer_height = int(infer_height)
        self.interval = int(interval)
        self.fade_time = int(fade_time)
        self.min_alpha = 0.2
        self.alpha_lut = self.build_alpha_lut(self.interval, self.fade_time)
        self.runtime_interval = len(self.alpha_lut)
        self.frame_count = 0
        self.cache = []

    def __call__(
        self,
        batch_meta,
        result,
        box_color=(0.0, 0.0, 1.0, 1.0),
        box_width=2,
        text_color=(1.0, 1.0, 1.0, 1.0),
        text_bg_color=(0.0, 0.0, 0.0, 0.6),
        kpt_color=(1.0, 1.0, 1.0, 1.0),
        kpt_bg_color=(0.0, 0.0, 1.0, 1.0),
        kpt_radius=6,
        kpt_line_color=(0.0, 0.0, 1.0, 1.0),
        kpt_line_width=2,
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
        faded_kpt_color = self.fade_color(kpt_color, fade_alpha)
        faded_kpt_bg_color = self.fade_color(kpt_bg_color, fade_alpha)
        faded_kpt_line_color = self.fade_color(kpt_line_color, fade_alpha)
        for item in self.cache:
            self.append_object(
                batch_meta,
                frame_meta,
                item,
                faded_box_color,
                box_width,
                faded_text_color,
                faded_text_bg_color,
                faded_kpt_color,
                faded_kpt_bg_color,
                faded_kpt_line_color,
                kpt_radius,
                kpt_line_width,
            )
