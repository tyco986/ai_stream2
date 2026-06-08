import logging
import struct

from pyservicemaker import BatchMetadataOperator, osd

logger = logging.getLogger(__name__)

MAX_ELEMENTS_IN_DISPLAY_META = 16
FLOAT_SIZE = struct.calcsize("f")

# COCO pose skeleton (1-based joint indices), same as DeepStream-Yolo-Pose sample.
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


class _YoloDrawerBase(BatchMetadataOperator):
    def __init__(
        self,
        border_color=(0.0, 1.0, 0.0, 1.0),
        border_width=2,
        show_labels=True,
    ):
        super().__init__()
        r, g, b, a = border_color
        self._border_color = osd.Color(float(r), float(g), float(b), float(a))
        self._text_color = osd.Color(1.0, 1.0, 1.0, 1.0)
        self._text_bg = osd.Color(0.0, 0.0, 0.0, 0.6)
        self._border_width = int(border_width)
        self._show_labels = bool(show_labels)

    @staticmethod
    def _iter_objects(batch_meta):
        for frame_meta in batch_meta.frame_items:
            yield frame_meta, frame_meta.object_items

    def _apply_bbox(self, obj):
        rect = obj.rect_params
        rect.border_width = self._border_width
        rect.border_color = self._border_color

    def _apply_label(self, obj):
        if not self._show_labels or not obj.label:
            return

        rect = obj.rect_params
        text = obj.text_params
        text.display_text = str(obj.label).encode("utf-8")
        text.x_offset = int(rect.left)
        text.y_offset = max(0, int(rect.top) - 14)
        text.font_params.name = osd.FontFamily.Serif
        text.font_params.size = 12
        text.font_params.color = self._text_color
        text.set_bg_clr = 1
        text.text_bg_clr = self._text_bg


class YoloDetectionDrawer(_YoloDrawerBase):
    """Style pgie object_meta for nvosdbin/nvdsosd (pyservicemaker osd colors).

    Bbox decode and letterbox are handled in pgie YAML (NvDsInferParseYolo).
    Confidence filtering uses pgie pre-cluster-threshold only.
    """

    def __init__(
        self,
        border_color=(0.0, 1.0, 0.0, 1.0),
        border_width=2,
        show_labels=True,
    ):
        super().__init__(border_color, border_width, show_labels)
        logger.info(
            "YoloDetectionDrawer: border_width=%d show_labels=%s",
            self._border_width,
            self._show_labels,
        )

    def handle_metadata(self, batch_meta):
        for _, objects in self._iter_objects(batch_meta):
            for obj in objects:
                self._apply_bbox(obj)
                self._apply_label(obj)
        return True


class YoloSegDrawer(_YoloDrawerBase):
    """Draw YOLO-Seg detections: bbox + instance mask + labels."""

    def __init__(
        self,
        border_color=(0.0, 1.0, 0.0, 1.0),
        border_width=2,
        show_labels=True,
        show_seg=True,
        mask_color=(0.0, 1.0, 0.0, 0.4),
        mask_threshold=0.5,
    ):
        super().__init__(border_color, border_width, show_labels)
        r, g, b, a = mask_color
        self._mask_color = osd.Color(float(r), float(g), float(b), float(a))
        self._mask_threshold = float(mask_threshold)
        self._show_seg = bool(show_seg)
        logger.info(
            "YoloSegDrawer: border_width=%d show_labels=%s show_seg=%s",
            self._border_width,
            self._show_labels,
            self._show_seg,
        )

    def handle_metadata(self, batch_meta):
        for _, objects in self._iter_objects(batch_meta):
            for obj in objects:
                self._apply_bbox(obj)
                if self._show_seg:
                    self._apply_seg_mask(obj)
                self._apply_label(obj)
        return True

    def _apply_seg_mask(self, obj):
        mask = obj.mask_params
        if mask.size <= 0:
            return
        mask.color = self._mask_color
        mask.threshold = self._mask_threshold


class YoloPoseDrawer(_YoloDrawerBase):
    """Draw YOLO-Pose detections: bbox + keypoints + labels."""

    def __init__(
        self,
        border_color=(0.0, 0.0, 1.0, 1.0),
        border_width=2,
        show_labels=True,
        show_kpts=True,
        kpt_color=(1.0, 1.0, 1.0, 1.0),
        kpt_bg_color=(0.0, 0.0, 1.0, 1.0),
        kpt_radius=6,
        kpt_line_color=(0.0, 0.0, 1.0, 1.0),
        kpt_line_width=2,
        kpt_conf_threshold=0.5,
    ):
        super().__init__(border_color, border_width, show_labels)
        r, g, b, a = kpt_color
        self._kpt_color = osd.Color(float(r), float(g), float(b), float(a))
        r, g, b, a = kpt_bg_color
        self._kpt_bg_color = osd.Color(float(r), float(g), float(b), float(a))
        r, g, b, a = kpt_line_color
        self._kpt_line_color = osd.Color(float(r), float(g), float(b), float(a))
        self._kpt_radius = int(kpt_radius)
        self._kpt_line_width = int(kpt_line_width)
        self._kpt_conf_threshold = float(kpt_conf_threshold)
        self._show_kpts = bool(show_kpts)
        logger.info(
            "YoloPoseDrawer: border_width=%d show_labels=%s show_kpts=%s",
            self._border_width,
            self._show_labels,
            self._show_kpts,
        )

    def handle_metadata(self, batch_meta):
        for frame_meta, objects in self._iter_objects(batch_meta):
            for obj in objects:
                self._apply_bbox(obj)
                if self._show_kpts:
                    self._draw_keypoints(batch_meta, frame_meta, obj)
                self._apply_label(obj)
        return True

    def _draw_keypoints(self, batch_meta, frame_meta, obj):
        mask = obj.mask_params
        if mask.size <= 0:
            return

        frame_w = frame_meta.pipeline_width
        frame_h = frame_meta.pipeline_height
        num_joints = mask.size // (FLOAT_SIZE * 3)
        if num_joints <= 0:
            return

        data = mask.mask_array
        gain = min(mask.width / frame_w, mask.height / frame_h)
        pad_x = (mask.width - frame_w * gain) * 0.5
        pad_y = (mask.height - frame_h * gain) * 0.5

        display_meta = None
        display_metas = []

        for joint_idx in range(num_joints):
            base = joint_idx * 3
            confidence = data[base + 2]
            if confidence < self._kpt_conf_threshold:
                continue

            xc = (data[base + 0] - pad_x) / gain
            yc = (data[base + 1] - pad_y) / gain
            display_meta = self._add_circle(batch_meta, frame_meta, display_meta, xc, yc)
            if display_meta not in display_metas:
                display_metas.append(display_meta)

        for joint_a, joint_b in YOLO_POSE_SKELETON:
            idx_a = (joint_a - 1) * 3
            idx_b = (joint_b - 1) * 3
            if idx_a + 2 >= len(data) or idx_b + 2 >= len(data):
                continue
            if (
                data[idx_a + 2] < self._kpt_conf_threshold
                or data[idx_b + 2] < self._kpt_conf_threshold
            ):
                continue

            x1 = (data[idx_a + 0] - pad_x) / gain
            y1 = (data[idx_a + 1] - pad_y) / gain
            x2 = (data[idx_b + 0] - pad_x) / gain
            y2 = (data[idx_b + 1] - pad_y) / gain
            display_meta = self._add_line(batch_meta, frame_meta, display_meta, x1, y1, x2, y2)
            if display_meta not in display_metas:
                display_metas.append(display_meta)

        for meta in display_metas:
            frame_meta.append(meta)

        self._clear_mask_params(mask)

    def _acquire_display_meta(self, batch_meta, display_meta, element_count, element_limit):
        if display_meta is not None and element_count < element_limit:
            return display_meta
        return batch_meta.acquire_display_meta()

    def _add_circle(self, batch_meta, frame_meta, display_meta, xc, yc):
        display_meta = self._acquire_display_meta(
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

    def _add_line(self, batch_meta, frame_meta, display_meta, x1, y1, x2, y2):
        display_meta = self._acquire_display_meta(
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

    @staticmethod
    def _clear_mask_params(mask):
        mask.size = 0
        mask.width = 0
        mask.height = 0
