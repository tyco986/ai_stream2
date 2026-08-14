from pathlib import Path

import onnx
import onnxslim
import torch
import torch.nn as nn
from ultralytics import YOLO

from utils.yolo_non_e2e.common import (
    fix_batch_only_dynamic,
    prepare_yolo_core,
    suppress_export_warnings,
    validate_export_args,
    write_labels,
)


class EfficientNmsTrt(torch.autograd.Function):
    """Stock TensorRT EfficientNMS_TRT (4 outputs)."""

    @staticmethod
    def forward(ctx, boxes, scores, conf, iou, max_det):
        batch_size = scores.shape[0]
        num_classes = scores.shape[-1]
        num_detections = torch.randint(0, max_det, (batch_size, 1), dtype=torch.int32)
        detection_boxes = torch.randn(batch_size, max_det, 4)
        detection_scores = torch.randn(batch_size, max_det)
        detection_classes = torch.randint(
            0, num_classes, (batch_size, max_det), dtype=torch.int32
        )
        return num_detections, detection_boxes, detection_scores, detection_classes

    @staticmethod
    def symbolic(g, boxes, scores, conf, iou, max_det):
        return g.op(
            "TRT::EfficientNMS_TRT",
            boxes,
            scores,
            score_threshold_f=conf,
            iou_threshold_f=iou,
            max_output_boxes_i=max_det,
            background_class_i=-1,
            score_activation_i=0,
            class_agnostic_i=0,
            box_coding_i=0,
            outputs=4,
        )


class DetNmsOutput(nn.Module):
    """Raw detect head → EfficientNMS_TRT → packed output0 [B, max_det, 6]."""

    def __init__(self, conf, iou, max_det):
        super().__init__()
        self.conf = conf
        self.iou = iou
        self.max_det = max_det

    def forward(self, x):
        preds = x[0] if isinstance(x, (list, tuple)) else x
        preds = preds.transpose(1, 2)
        boxes = preds[:, :, :4]
        scores = preds[:, :, 4:]
        num_detections, detection_boxes, detection_scores, detection_classes = EfficientNmsTrt.apply(
            boxes,
            scores,
            self.conf,
            self.iou,
            self.max_det,
        )
        result = torch.cat(
            [
                detection_boxes,
                detection_scores.unsqueeze(-1),
                detection_classes.unsqueeze(-1).to(detection_boxes.dtype),
            ],
            dim=-1,
        )
        return result


class YoloDetNonE2EExporter:
    """YOLO11 detect: EfficientNMS_TRT → images/output0 for NvDsInferYoloE2E."""

    def export(
        self,
        weights: Path,
        size: int,
        opset: int,
        batch: int,
        dynamic: bool,
        simplify: bool,
        conf: float,
        iou: float,
        max_det: int,
        output_dir: Path,
    ) -> None:
        suppress_export_warnings()
        validate_export_args(weights, dynamic, batch)
        yolo = YOLO(str(weights))
        write_labels(yolo.names, output_dir / "labels.txt")

        device = torch.device("cpu")
        core = prepare_yolo_core(weights, device, max_det)
        model = nn.Sequential(
            core,
            DetNmsOutput(conf, iou, max_det),
        )
        onnx_path = output_dir / f"{weights.stem}.onnx"
        onnx_input = torch.zeros(batch, 3, size, size, device=device)
        dynamic_axes = None
        if dynamic:
            dynamic_axes = {"images": {0: "batch"}, "output0": {0: "batch"}}

        torch.onnx.export(
            model,
            onnx_input,
            str(onnx_path),
            verbose=False,
            opset_version=opset,
            do_constant_folding=True,
            input_names=["images"],
            output_names=["output0"],
            dynamic_axes=dynamic_axes,
            dynamo=False,
        )
        if simplify:
            onnx.save(onnxslim.slim(onnx.load(str(onnx_path))), str(onnx_path))
        if dynamic:
            fix_batch_only_dynamic(onnx_path, size, max_det)
