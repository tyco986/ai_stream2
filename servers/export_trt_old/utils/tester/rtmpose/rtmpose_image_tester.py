from pathlib import Path

import cupy as cp
import cv2

from utils.tester.rtmpose.constants import (
    BBOX_PADDING,
    BOX_WIDTH,
    COCO17_EDGES,
    COLOR_GREEN_BGR,
    COLOR_ORANGE_BGR,
    COLOR_RED_BGR,
    DEFAULT_DETECTOR_ENGINE,
    DET_THRESHOLD,
    FONT_SCALE,
    FONT_THICKNESS,
    KPT_RADIUS,
    LABEL_SEP,
    LABEL_Y_OFFSET,
    OUTPUT_DIR,
    PERSON_CLASS,
    PERSON_LABEL,
    POSE_MEAN,
    POSE_SCALE,
    SKELETON_WIDTH,
    YOLO_SCALE,
)
from utils.tester.trt_runner import TrtRunner


class RtmposeImageTester:
    def __init__(
        self,
        rtmpose,
        pose_threshold,
        detector=DEFAULT_DETECTOR_ENGINE,
    ):
        self.rtmpose = rtmpose
        self.pose_threshold = pose_threshold
        self.detector = detector
        self.det_runner = TrtRunner(detector)
        self.pose_runner = TrtRunner(rtmpose)
        self.det_h, self.det_w = self.det_runner.input_shape[2:4]
        self.pose_h, self.pose_w = self.pose_runner.input_shape[2:4]
        self.pose_mean = cp.asarray(POSE_MEAN, dtype=cp.float32)

    def letterbox(self, bgr, height, width):
        src_h, src_w = bgr.shape[:2]
        scale = min(width / src_w, height / src_h)
        new_w = min(width, int(src_w * scale + 0.5))
        new_h = min(height, int(src_h * scale + 0.5))
        resized = cv2.resize(bgr, (new_w, new_h), interpolation=cv2.INTER_LINEAR)
        canvas = cp.zeros((height, width, 3), dtype=cp.float32)
        pad_x = (width - new_w) // 2
        pad_y = (height - new_h) // 2
        canvas[pad_y : pad_y + new_h, pad_x : pad_x + new_w] = cp.asarray(resized)
        return canvas, scale, pad_x, pad_y, new_w, new_h

    def detect_persons(self, bgr):
        src_h, src_w = bgr.shape[:2]
        canvas, scale, pad_x, pad_y, _, _ = self.letterbox(bgr, self.det_h, self.det_w)
        nchw = cp.transpose(canvas[:, :, ::-1] * YOLO_SCALE, (2, 0, 1))[None]
        dets = self.det_runner(nchw).reshape(-1, 6)
        dets = dets[
            (dets[:, 4] >= DET_THRESHOLD) & (dets[:, 5].astype(cp.int32) == PERSON_CLASS)
        ]
        boxes = cp.stack(
            (
                cp.clip((dets[:, 0] - pad_x) / scale, 0, src_w),
                cp.clip((dets[:, 1] - pad_y) / scale, 0, src_h),
                cp.clip((dets[:, 2] - pad_x) / scale, 0, src_w),
                cp.clip((dets[:, 3] - pad_y) / scale, 0, src_h),
                dets[:, 4],
            ),
            axis=1,
        )
        return boxes

    def expand_box(self, x1, y1, x2, y2, frame_w, frame_h):
        width = x2 - x1
        height = y2 - y1
        aspect = self.pose_w / float(self.pose_h)
        cx = x1 + width * 0.5
        cy = y1 + height * 0.5
        scale_w = width * BBOX_PADDING
        scale_h = height * BBOX_PADDING
        if scale_w > aspect * scale_h:
            scale_h = scale_w / aspect
        else:
            scale_w = scale_h * aspect
        left = max(0.0, cx - scale_w * 0.5)
        top = max(0.0, cy - scale_h * 0.5)
        right = min(float(frame_w), cx + scale_w * 0.5)
        bottom = min(float(frame_h), cy + scale_h * 0.5)
        geom = (left, top, max(2.0, right - left), max(2.0, bottom - top))
        return geom

    def pose_tensor(self, bgr, left, top, src_w, src_h):
        x0, y0 = int(left), int(top)
        crop = bgr[y0 : int(top + src_h), x0 : int(left + src_w)]
        canvas, _, pad_x, pad_y, dest_w, dest_h = self.letterbox(
            crop, self.pose_h, self.pose_w
        )
        nchw = cp.transpose((canvas[:, :, ::-1] - self.pose_mean) * POSE_SCALE, (2, 0, 1))
        pack = (
            nchw,
            float(x0),
            float(y0),
            float(crop.shape[1]),
            float(crop.shape[0]),
            float(dest_w),
            float(dest_h),
            float(pad_x),
            float(pad_y),
        )
        return pack

    def map_keypoints(self, kpts, left, top, src_w, src_h, dest_w, dest_h, pad_x, pad_y):
        mapped = cp.asarray(kpts, dtype=cp.float32)
        mapped[:, 0] = left + (mapped[:, 0] - pad_x) / (dest_w / src_w)
        mapped[:, 1] = top + (mapped[:, 1] - pad_y) / (dest_h / src_h)
        return mapped

    def draw_det(self, canvas, x1, y1, x2, y2, conf):
        left, top, right, bottom = int(x1), int(y1), int(x2), int(y2)
        cv2.rectangle(canvas, (left, top), (right, bottom), COLOR_GREEN_BGR, BOX_WIDTH)
        cv2.putText(
            canvas,
            f"{PERSON_LABEL}{LABEL_SEP}{float(conf):.2f}",
            (left, max(0, top - LABEL_Y_OFFSET)),
            cv2.FONT_HERSHEY_SIMPLEX,
            FONT_SCALE,
            COLOR_GREEN_BGR,
            FONT_THICKNESS,
        )

    def draw_pose(self, canvas, kpts):
        host = cp.asnumpy(kpts)
        n = host.shape[0]
        for i, j in (edge for edge in COCO17_EDGES if edge[0] < n and edge[1] < n):
            p1 = (int(host[i, 0]), int(host[i, 1]))
            p2 = (int(host[j, 0]), int(host[j, 1]))
            cv2.line(canvas, p1, p2, COLOR_ORANGE_BGR, SKELETON_WIDTH)
        for x, y, score in host:
            color = COLOR_RED_BGR if score < self.pose_threshold else COLOR_ORANGE_BGR
            cv2.circle(canvas, (int(x), int(y)), KPT_RADIUS, color, -1)

    def annotate(self, bgr):
        frame_h, frame_w = bgr.shape[:2]
        canvas = bgr.copy()
        boxes = self.detect_persons(bgr)
        packs = [
            self.pose_tensor(
                bgr,
                *self.expand_box(float(x1), float(y1), float(x2), float(y2), frame_w, frame_h),
            )
            for x1, y1, x2, y2, conf in boxes
        ]
        kpts_batch = (
            self.pose_runner(cp.stack([pack[0] for pack in packs], axis=0)).reshape(
                len(packs), -1, 3
            )
            if packs
            else ()
        )
        for (x1, y1, x2, y2, conf), pack, kpts in zip(boxes, packs, kpts_batch):
            self.draw_det(canvas, x1, y1, x2, y2, conf)
            self.draw_pose(canvas, self.map_keypoints(kpts, *pack[1:]))
        return canvas

    def __call__(self, image_path):
        path = Path(image_path)
        bgr = cv2.imread(str(path), cv2.IMREAD_COLOR)
        if bgr is None:
            raise ValueError(f"failed to read image: {path}")
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        out_path = OUTPUT_DIR / path.name
        cv2.imwrite(str(out_path), self.annotate(bgr))
        return out_path
