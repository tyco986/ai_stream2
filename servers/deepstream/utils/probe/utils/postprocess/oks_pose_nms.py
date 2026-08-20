import cupy

# COCO 17-keypoint OKS sigmas (xtcocotools / MMPose).
COCO17_SIGMAS = (
    0.026,
    0.025,
    0.025,
    0.035,
    0.035,
    0.079,
    0.079,
    0.072,
    0.072,
    0.062,
    0.062,
    0.107,
    0.107,
    0.087,
    0.087,
    0.089,
    0.089,
)


class OksPoseNms:
    """Crop-space keypoints stay on GPU through affine + OKS-NMS; one D2H at the end.

    Inputs must already be CuPy device arrays. Host arrays are uploaded once at
    apply() entry (H2D), then all math stays on GPU until the return asnumpy.
    """

    def __init__(
        self,
        infer_height=256,
        infer_width=192,
        oks_thr=0.5,
        vis_thr=0.0,
        sigmas=None,
    ):
        self.infer_height = int(infer_height)
        self.infer_width = int(infer_width)
        self.oks_thr = float(oks_thr)
        self.vis_thr = float(vis_thr)
        self.sigmas = COCO17_SIGMAS if sigmas is None else sigmas
        self.sigmas_gpu = cupy.asarray(self.sigmas, dtype=cupy.float32)
        self.area_eps = cupy.float32(1e-9)

    def align_even(self, left, top, width, height):
        src_left = (left.astype(cupy.int32) + 1) & ~cupy.int32(1)
        src_top = (top.astype(cupy.int32) + 1) & ~cupy.int32(1)
        src_width = cupy.maximum(2, width.astype(cupy.int32) & ~cupy.int32(1))
        src_height = cupy.maximum(2, height.astype(cupy.int32) & ~cupy.int32(1))
        return src_left, src_top, src_width, src_height

    def letterbox_params(self, src_width, src_height):
        src_width_f = src_width.astype(cupy.float32)
        src_height_f = src_height.astype(cupy.float32)
        fit_height = self.infer_width * src_height_f / src_width_f
        use_full_height = fit_height > self.infer_height
        dest_width = cupy.where(
            use_full_height,
            (self.infer_height * src_width_f / src_height_f).astype(cupy.int32),
            cupy.int32(self.infer_width),
        )
        dest_height = cupy.where(
            use_full_height,
            cupy.int32(self.infer_height),
            fit_height.astype(cupy.int32),
        )
        offset_left = (self.infer_width - dest_width) // 2
        offset_top = (self.infer_height - dest_height) // 2
        ratio_x = dest_width.astype(cupy.float32) / src_width_f
        ratio_y = dest_height.astype(cupy.float32) / src_height_f
        return offset_left, offset_top, ratio_x, ratio_y

    def map_to_frame(self, keypoints, rects):
        src_left, src_top, src_width, src_height = self.align_even(
            rects[:, 0],
            rects[:, 1],
            rects[:, 2],
            rects[:, 3],
        )
        offset_left, offset_top, ratio_x, ratio_y = self.letterbox_params(
            src_width,
            src_height,
        )
        src_left_f = src_left.astype(cupy.float32)[:, None]
        src_top_f = src_top.astype(cupy.float32)[:, None]
        offset_left_f = offset_left.astype(cupy.float32)[:, None]
        offset_top_f = offset_top.astype(cupy.float32)[:, None]
        ratio_x_f = ratio_x[:, None]
        ratio_y_f = ratio_y[:, None]
        frame_x = src_left_f + (keypoints[:, :, 0] - offset_left_f) / ratio_x_f
        frame_y = src_top_f + (keypoints[:, :, 1] - offset_top_f) / ratio_y_f
        mapped = cupy.stack((frame_x, frame_y, keypoints[:, :, 2]), axis=2)
        return mapped

    def keypoint_vars(self, num_keypoints):
        sigmas = self.sigmas_gpu
        if int(sigmas.shape[0]) != num_keypoints:
            sigmas = cupy.full((num_keypoints,), 0.05, dtype=cupy.float32)
        vars_k = (sigmas * cupy.float32(2.0)) ** 2
        return vars_k

    def pairwise_oks(self, keypoints, areas):
        vars_k = self.keypoint_vars(int(keypoints.shape[1]))
        dx = keypoints[:, None, :, 0] - keypoints[None, :, :, 0]
        dy = keypoints[:, None, :, 1] - keypoints[None, :, :, 1]
        scale = (areas[:, None] + areas[None, :]) * cupy.float32(0.5) + self.area_eps
        energy = (dx * dx + dy * dy) / vars_k / scale[:, :, None] / cupy.float32(2.0)
        kernel = cupy.exp(-energy)
        visible = (keypoints[:, None, :, 2] > self.vis_thr) & (
            keypoints[None, :, :, 2] > self.vis_thr
        )
        denom = cupy.maximum(visible.sum(axis=2), 1)
        oks = (kernel * visible).sum(axis=2) / denom
        eye = cupy.eye(int(keypoints.shape[0]), dtype=cupy.bool_)
        oks = cupy.where(eye, cupy.float32(1.0), oks)
        return oks

    def greedy_keep(self, oks, scores):
        count = int(scores.shape[0])
        order = cupy.argsort(-scores)
        keep_sorted = cupy.ones((count,), dtype=cupy.bool_)
        oks_sorted = oks[order][:, order]
        positions = cupy.arange(count)
        for index in range(count):
            suppress = (oks_sorted[index] > self.oks_thr) & keep_sorted[index]
            suppress = suppress & (positions > index)
            keep_sorted = keep_sorted & ~suppress
        keep = order[keep_sorted]
        return keep

    def apply(self, keypoints, rects, scores=None):
        keypoints = cupy.asarray(keypoints, dtype=cupy.float32)
        rects = cupy.asarray(rects, dtype=cupy.float32)
        rank_scores = scores
        if rank_scores is not None:
            rank_scores = cupy.asarray(rank_scores, dtype=cupy.float32)
        count = int(keypoints.shape[0])
        num_keypoints = int(keypoints.shape[1]) if keypoints.ndim == 3 else 0
        keep = cupy.zeros((0,), dtype=cupy.int64)
        mapped = cupy.zeros((0, num_keypoints, 3), dtype=cupy.float32)
        if count > 0:
            mapped = self.map_to_frame(keypoints, rects)
            areas = rects[:, 2] * rects[:, 3]
            if rank_scores is None:
                rank_scores = mapped[:, :, 2].mean(axis=1)
            oks = self.pairwise_oks(mapped, areas)
            keep = self.greedy_keep(oks, rank_scores)
            mapped = mapped[keep]
        keep_host = cupy.asnumpy(keep)
        keypoints_host = cupy.asnumpy(mapped)
        return {
            "keep": keep_host,
            "keypoints": keypoints_host,
        }
