"""TensorRT EfficientNMSX_TRT custom plugin (5 outputs, includes detection indices)."""

import torch


class EfficientNmsXTrt(torch.autograd.Function):
    """ONNX symbolic → TRT::EfficientNMSX_TRT.

    Outputs (per batch, padded to max_det):
      num_detections, detection_boxes, detection_scores, detection_classes,
      detections_indices  — indices into the pre-NMS proposal axis for gather.
    """

    @staticmethod
    def forward(ctx, boxes, scores, score_threshold, iou_threshold, max_output_boxes):
        batch_size = scores.shape[0]
        num_classes = scores.shape[-1]
        num_detections = torch.randint(
            0, max_output_boxes, (batch_size, 1), dtype=torch.int32
        )
        detection_boxes = torch.randn(batch_size, max_output_boxes, 4)
        detection_scores = torch.randn(batch_size, max_output_boxes)
        detection_classes = torch.randint(
            0, num_classes, (batch_size, max_output_boxes), dtype=torch.int32
        )
        detections_indices = torch.randint(
            0, max_output_boxes, (batch_size, max_output_boxes), dtype=torch.int32
        )
        return (
            num_detections,
            detection_boxes,
            detection_scores,
            detection_classes,
            detections_indices,
        )

    @staticmethod
    def symbolic(g, boxes, scores, score_threshold, iou_threshold, max_output_boxes):
        return g.op(
            "TRT::EfficientNMSX_TRT",
            boxes,
            scores,
            score_threshold_f=score_threshold,
            iou_threshold_f=iou_threshold,
            max_output_boxes_i=max_output_boxes,
            background_class_i=-1,
            score_activation_i=0,
            class_agnostic_i=0,
            box_coding_i=0,
            outputs=5,
        )
