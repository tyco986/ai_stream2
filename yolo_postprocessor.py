import logging
import os

import cupy as cp
from pyservicemaker import BatchMetadataOperator, osd

logger = logging.getLogger(__name__)

# YOLOv10 output: [batch, 300, 6] where each row = (x1, y1, x2, y2, conf, class_id)
# Coordinates are in model input space (640x640 with letterboxing).
NETWORK_SIZE = 640
PERSON_CLASS_ID = 0


class YoloV10Postprocessor(BatchMetadataOperator):
    """Parse YOLOv10 post-NMS tensor output and inject obj_meta into frames.

    Attaches as a probe after nvinfer with output-tensor-meta=1 and cluster-mode=4.

    All filtering and coordinate transforms run on GPU via CuPy.  Only the
    final (small) array of surviving detections is copied to CPU to populate
    DeepStream ObjectMetadata — this is the minimum necessary transfer.
    """

    def __init__(self, confidence_threshold=0.3, person_only=True, labels_path=None):
        super().__init__()
        self._threshold = confidence_threshold
        self._person_only = person_only
        self._labels = self._load_labels(labels_path) if labels_path else {}
        self._color = osd.Color(0.0, 1.0, 0.0, 1.0)
        logger.info(
            "YoloV10Postprocessor: threshold=%.2f person_only=%s labels=%d",
            self._threshold, self._person_only, len(self._labels),
        )

    def handle_metadata(self, batch_meta):
        for frame_meta in batch_meta.frame_items:
            # nvinfer receives frames at pipeline resolution (set by
            # nvmultiurisrcbin width/height), NOT the original source
            # resolution.  The letterbox transform happens from pipeline
            # dims → model input (640×640), so we must undo it using
            # pipeline_width/pipeline_height.
            pipe_w = frame_meta.pipeline_width
            pipe_h = frame_meta.pipeline_height
            if not pipe_w or not pipe_h:
                logger.error("pipeline_width/height is 0 — skipping frame")
                continue

            for tensor_meta in frame_meta.tensor_items:
                layers = tensor_meta.as_tensor_output().get_layers()
                detections = self._parse_output_gpu(layers, pipe_w, pipe_h)

                for cls_id, conf, left, top, width, height in detections:
                    obj_meta = batch_meta.acquire_object_meta()
                    obj_meta.class_id = int(cls_id)
                    obj_meta.confidence = float(conf)
                    obj_meta.label = self._labels.get(int(cls_id), str(int(cls_id)))

                    rect = osd.Rect()
                    rect.left = float(left)
                    rect.top = float(top)
                    rect.width = float(width)
                    rect.height = float(height)
                    rect.border_width = 2
                    rect.border_color = self._color
                    obj_meta.rect_params = rect

                    frame_meta.append(obj_meta)

        return True

    # ------------------------------------------------------------------
    # GPU-accelerated tensor parsing
    # ------------------------------------------------------------------

    def _parse_output_gpu(self, layers, pipe_w, pipe_h):
        """Filter and transform YOLOv10 [300, 6] tensor entirely on GPU.

        nvinfer with maintain-aspect-ratio=1 scales the frame to fit inside
        the model input (640×640) while preserving aspect ratio, aligned to
        the top-left corner (no centering).  We undo that transform here.

        Returns a small CPU list of (cls_id, conf, left, top, w, h) tuples
        — only the surviving detections are transferred from GPU to CPU.
        """
        # Grab the first (and only) output layer as a CuPy GPU array.
        tensor = None
        for _name, layer in layers.items():
            tensor = cp.from_dlpack(layer)
            break

        if tensor is None:
            return []

        # Squeeze batch dim: (1, 300, 6) → (300, 6)
        if tensor.ndim == 3:
            tensor = tensor[0]

        # ── GPU: confidence filter ────────────────────────────────────
        conf = tensor[:, 4]
        mask = conf >= self._threshold

        # ── GPU: optional class filter ────────────────────────────────
        if self._person_only:
            mask &= tensor[:, 5] == PERSON_CLASS_ID

        filtered = tensor[mask]  # still on GPU, shape (N, 6)

        if filtered.shape[0] == 0:
            return []

        # ── GPU: undo nvinfer letterbox (symmetric/centered padding) ──
        # With symmetric-padding=1, nvinfer centers the scaled image inside
        # the 640×640 canvas.  We subtract the padding then divide by scale.
        scale = min(NETWORK_SIZE / pipe_w, NETWORK_SIZE / pipe_h)
        pad_x = (NETWORK_SIZE - pipe_w * scale) / 2.0
        pad_y = (NETWORK_SIZE - pipe_h * scale) / 2.0

        x1 = (filtered[:, 0] - pad_x) / scale
        y1 = (filtered[:, 1] - pad_y) / scale
        x2 = (filtered[:, 2] - pad_x) / scale
        y2 = (filtered[:, 3] - pad_y) / scale

        # ── GPU: clamp to frame bounds ────────────────────────────────
        x1 = cp.clip(x1, 0.0, pipe_w)
        y1 = cp.clip(y1, 0.0, pipe_h)
        x2 = cp.clip(x2, 0.0, pipe_w)
        y2 = cp.clip(y2, 0.0, pipe_h)

        w = x2 - x1
        h = y2 - y1

        # ── GPU: size filter ─────────────────────────────────────────
        valid = (w >= 1.0) & (h >= 1.0)
        if not cp.any(valid):
            return []

        cls_ids = filtered[valid, 5]
        confs = filtered[valid, 4]
        x1, y1, w, h = x1[valid], y1[valid], w[valid], h[valid]

        # ── Single GPU→CPU transfer: only surviving detections ────────
        result = cp.stack([cls_ids, confs, x1, y1, w, h], axis=1).get()

        return [tuple(row) for row in result]

    @staticmethod
    def _load_labels(path):
        if not os.path.isfile(path):
            return {}
        labels = {}
        with open(path) as f:
            for idx, line in enumerate(f):
                name = line.strip()
                if name:
                    labels[idx] = name
        return labels
