"""TensorRT ROIAlignX_TRT custom plugin for instance-seg proto pooling."""

import torch


class RoiAlignXTrt(torch.autograd.Function):
    """ONNX symbolic → TRT::ROIAlignX_TRT.

    Args mirror DeepStream-Yolo-Seg export:
      X: feature map [B, C, H, W]
      rois: boxes [N, 4] in feature/input pixel space
      batch_indices: [N] int32 batch id per roi
    """

    @staticmethod
    def forward(
        ctx,
        features,
        rois,
        batch_indices,
        coordinate_transformation_mode,
        mode,
        output_height,
        output_width,
        sampling_ratio,
        spatial_scale,
    ):
        num_channels = features.shape[1]
        num_rois = rois.shape[0]
        return torch.randn(
            [num_rois, num_channels, output_height, output_width],
            device=rois.device,
            dtype=rois.dtype,
        )

    @staticmethod
    def symbolic(
        g,
        features,
        rois,
        batch_indices,
        coordinate_transformation_mode,
        mode,
        output_height,
        output_width,
        sampling_ratio,
        spatial_scale,
    ):
        return g.op(
            "TRT::ROIAlignX_TRT",
            features,
            rois,
            batch_indices,
            coordinate_transformation_mode_i=coordinate_transformation_mode,
            mode_i=mode,
            output_height_i=output_height,
            output_width_i=output_width,
            sampling_ratio_i=sampling_ratio,
            spatial_scale_f=spatial_scale,
        )
