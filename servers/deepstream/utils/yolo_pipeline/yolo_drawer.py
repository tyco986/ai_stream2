from pyservicemaker import BatchMetadataOperator, osd

MAX_ELEMENTS_IN_DISPLAY_META = 16
POSE_KEYPOINT_COUNT = 17

YOLO_POSE_SKELETON = [
    [16, 14],
    [14, 12],
    [17, 15],
    [15, 13],
    [12, 13],
    [6, 12],
    [7, 13],
    [6, 7],
    [6, 8],
    [7, 9],
    [8, 10],
    [9, 11],
    [2, 3],
    [1, 2],
    [1, 3],
    [2, 4],
    [3, 5],
    [4, 6],
    [5, 7],
]


class YoloDrawerBase(BatchMetadataOperator):
    def __init__(
        self,
        border_color=(0.0, 1.0, 0.0, 1.0),
        border_width=2,
        show_labels=True,
        show_conf=True,
    ):
        super().__init__()
        r, g, b, a = border_color
        self._border_color = osd.Color(float(r), float(g), float(b), float(a))
        self._text_color = osd.Color(1.0, 1.0, 1.0, 1.0)
        self._text_bg = osd.Color(0.0, 0.0, 0.0, 0.6)
        self._border_width = int(border_width)
        self._show_labels = bool(show_labels)
        self._show_conf = bool(show_conf)

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            for obj in frame_meta.object_items:
                self.apply_bbox(obj)
                self.decorate_object(batch_meta, frame_meta, obj)
                self.apply_label(obj)
        return True

    def decorate_object(self, batch_meta, frame_meta, obj):
        return

    def apply_bbox(self, obj):
        rect = obj.rect_params
        rect.rotation_angle = 0.0
        rect.border_width = self._border_width
        rect.border_color = self._border_color

    def apply_label(self, obj):
        if not self._show_labels or not obj.label:
            return

        rect = obj.rect_params
        text = obj.text_params
        label = str(obj.label)
        if self._show_conf:
            label = f"{label} {float(obj.confidence):.2f}"
        text.display_text = label.encode("utf-8")
        text.x_offset = int(rect.left)
        text.y_offset = max(0, int(rect.top) - 14)
        text.font_params.name = osd.FontFamily.Serif
        text.font_params.size = 12
        text.font_params.color = self._text_color
        text.set_bg_clr = 1
        text.text_bg_clr = self._text_bg


class YoloSegDrawer(YoloDrawerBase):
    pass


class YoloPoseDrawer(YoloDrawerBase):
    def __init__(
        self,
        infer_width,
        infer_height,
        border_color=(0.0, 0.0, 1.0, 1.0),
        border_width=2,
        show_labels=True,
        show_conf=True,
        kpt_color=(1.0, 1.0, 1.0, 1.0),
        kpt_bg_color=(0.0, 0.0, 1.0, 1.0),
        kpt_radius=6,
        kpt_line_color=(0.0, 0.0, 1.0, 1.0),
        kpt_line_width=2,
    ):
        super().__init__(border_color, border_width, show_labels, show_conf)
        self.infer_width = int(infer_width)
        self.infer_height = int(infer_height)
        r, g, b, a = kpt_color
        self._kpt_color = osd.Color(float(r), float(g), float(b), float(a))
        r, g, b, a = kpt_bg_color
        self._kpt_bg_color = osd.Color(float(r), float(g), float(b), float(a))
        r, g, b, a = kpt_line_color
        self._kpt_line_color = osd.Color(float(r), float(g), float(b), float(a))
        self._kpt_radius = int(kpt_radius)
        self._kpt_line_width = int(kpt_line_width)

    def decorate_object(self, batch_meta, frame_meta, obj):
        mask = obj.mask_params
        flat = [float(v) for v in mask.mask_array.ravel()[: POSE_KEYPOINT_COUNT * 3]]
        if not flat:
            return

        frame_w = frame_meta.pipeline_width
        frame_h = frame_meta.pipeline_height
        gain = min(self.infer_width / frame_w, self.infer_height / frame_h)
        pad_x = (self.infer_width - frame_w * gain) * 0.5
        pad_y = (self.infer_height - frame_h * gain) * 0.5

        display_meta = None
        display_metas = []

        for joint_idx in range(len(flat) // 3):
            base = joint_idx * 3
            xc = (flat[base + 0] - pad_x) / gain
            yc = (flat[base + 1] - pad_y) / gain
            display_meta = self.add_circle(batch_meta, frame_meta, display_meta, xc, yc)
            if display_meta not in display_metas:
                display_metas.append(display_meta)

        for joint_a, joint_b in YOLO_POSE_SKELETON:
            idx_a = (joint_a - 1) * 3
            idx_b = (joint_b - 1) * 3
            if idx_a + 2 >= len(flat) or idx_b + 2 >= len(flat):
                continue
            x1 = (flat[idx_a + 0] - pad_x) / gain
            y1 = (flat[idx_a + 1] - pad_y) / gain
            x2 = (flat[idx_b + 0] - pad_x) / gain
            y2 = (flat[idx_b + 1] - pad_y) / gain
            display_meta = self.add_line(batch_meta, frame_meta, display_meta, x1, y1, x2, y2)
            if display_meta not in display_metas:
                display_metas.append(display_meta)

        for meta in display_metas:
            frame_meta.append(meta)

    def acquire_display_meta(self, batch_meta, display_meta, element_count, element_limit):
        if display_meta is not None and element_count < element_limit:
            return display_meta
        return batch_meta.acquire_display_meta()

    def add_circle(self, batch_meta, frame_meta, display_meta, xc, yc):
        display_meta = self.acquire_display_meta(
            batch_meta,
            display_meta,
            display_meta.n_circles if display_meta is not None else 0,
            MAX_ELEMENTS_IN_DISPLAY_META,
        )
        frame_w = frame_meta.pipeline_width
        frame_h = frame_meta.pipeline_height
        circle = osd.Circle()
        circle.xc = int(min(frame_w - 1, max(0, xc)))
        circle.yc = int(min(frame_h - 1, max(0, yc)))
        circle.radius = self._kpt_radius
        circle.color = self._kpt_color
        circle.has_bg_color = 1
        circle.bg_color = self._kpt_bg_color
        display_meta.add_circle(circle)
        return display_meta

    def add_line(self, batch_meta, frame_meta, display_meta, x1, y1, x2, y2):
        display_meta = self.acquire_display_meta(
            batch_meta,
            display_meta,
            display_meta.n_lines if display_meta is not None else 0,
            MAX_ELEMENTS_IN_DISPLAY_META,
        )
        frame_w = frame_meta.pipeline_width
        frame_h = frame_meta.pipeline_height
        line = osd.Line()
        line.x1 = int(min(frame_w - 1, max(0, x1)))
        line.y1 = int(min(frame_h - 1, max(0, y1)))
        line.x2 = int(min(frame_w - 1, max(0, x2)))
        line.y2 = int(min(frame_h - 1, max(0, y2)))
        line.width = self._kpt_line_width
        line.color = self._kpt_line_color
        display_meta.add_line(line)
        return display_meta
