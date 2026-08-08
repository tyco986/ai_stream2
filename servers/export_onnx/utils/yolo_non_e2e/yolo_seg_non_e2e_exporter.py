from pathlib import Path

import onnx
import onnxslim
import torch
import torch.nn as nn
from ultralytics import YOLO

from modules import EfficientNmsXTrt, RoiAlignXTrt
from utils.yolo_non_e2e.common import (
    prepare_yolo_core,
    run_export_cli,
    set_dim_value,
    suppress_export_warnings,
    write_labels,
)


class SegMaskOutput(nn.Module):
    """EfficientNMSX + ROIAlignX → num_dets/det_boxes/det_scores/det_classes/det_masks."""

    def __init__(self, num_classes, conf, iou, max_det, spatial_scale=0.25):
        super().__init__()
        self.num_classes = num_classes
        self.conf = conf
        self.iou = iou
        self.max_det = max_det
        self.spatial_scale = spatial_scale

    def forward(self, x):
        preds = x[0].transpose(1, 2)
        boxes = preds[:, :, :4]
        scores = preds[:, :, 4 : 4 + self.num_classes]
        coeffs = preds[:, :, 4 + self.num_classes :]
        protos = x[1]

        num_dets, detection_boxes, detection_scores, detection_classes, detections_indices = (
            EfficientNmsXTrt.apply(boxes, scores, self.conf, self.iou, self.max_det)
        )

        batch_size, num_protos, h_protos, w_protos = protos.shape
        total_detections = batch_size * self.max_det

        batch_index = (
            torch.ones_like(detections_indices)
            * torch.arange(batch_size, device=boxes.device, dtype=torch.int32).unsqueeze(1)
        )
        batch_index = batch_index.view(total_detections).to(torch.int32)
        box_index = detections_indices.view(total_detections).to(torch.int32)

        selected_boxes = boxes[batch_index, box_index]
        selected_coeffs = coeffs[batch_index, box_index]

        pooled_proto = RoiAlignXTrt.apply(
            protos,
            selected_boxes,
            batch_index,
            1,
            1,
            int(h_protos),
            int(w_protos),
            0,
            self.spatial_scale,
        )
        masks_protos = torch.matmul(
            selected_coeffs.unsqueeze(1),
            pooled_proto.view(total_detections, num_protos, h_protos * w_protos),
        )
        det_masks = masks_protos.sigmoid().view(
            batch_size, self.max_det, h_protos * w_protos
        )
        result = (
            num_dets,
            detection_boxes,
            detection_scores,
            detection_classes,
            det_masks,
        )
        return result


class YoloSegNonE2EExporter:
    """YOLO11-Seg: EfficientNMSX_TRT + ROIAlignX_TRT → NvDsInferYoloMask blobs."""

    def fix_batch_only_dynamic(self, onnx_path: Path, size: int, max_det: int) -> None:
        model = onnx.load(str(onnx_path))
        inp = model.graph.input[0]
        dims = inp.type.tensor_type.shape.dim
        if len(dims) != 4:
            raise ValueError(f"expected NCHW input, got {len(dims)} dims")
        set_dim_value(dims[1], 3)
        set_dim_value(dims[2], size)
        set_dim_value(dims[3], size)

        outputs = list(model.graph.output)
        if len(outputs) != 5:
            raise ValueError(f"expected 5 outputs, got {len(outputs)}")
        mask_size = size // 4
        mask_flat = mask_size * mask_size
        expected = [
            ("num_dets", [None, 1]),
            ("det_boxes", [None, max_det, 4]),
            ("det_scores", [None, max_det]),
            ("det_classes", [None, max_det]),
            ("det_masks", [None, max_det, mask_flat]),
        ]
        for output, (name, shape) in zip(outputs, expected):
            if output.name != name:
                raise ValueError(f"expected output {name!r}, got {output.name!r}")
            out_dims = output.type.tensor_type.shape.dim
            if len(out_dims) != len(shape):
                raise ValueError(
                    f"{name}: expected rank {len(shape)}, got {len(out_dims)}"
                )
            for dim, value in zip(out_dims, shape):
                if value is not None:
                    set_dim_value(dim, value)
        onnx.save(model, str(onnx_path))

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
        yolo = YOLO(str(weights))
        write_labels(yolo.names, output_dir / "labels.txt")

        device = torch.device("cpu")
        core = prepare_yolo_core(weights, device)
        model = nn.Sequential(
            core,
            SegMaskOutput(len(yolo.names), conf, iou, max_det),
        )
        onnx_path = output_dir / f"{weights.stem}.onnx"
        onnx_input = torch.zeros(batch, 3, size, size, device=device)
        output_names = ["num_dets", "det_boxes", "det_scores", "det_classes", "det_masks"]
        dynamic_axes = None
        if dynamic:
            dynamic_axes = {name: {0: "batch"} for name in ["images", *output_names]}

        torch.onnx.export(
            model,
            onnx_input,
            str(onnx_path),
            verbose=False,
            opset_version=opset,
            do_constant_folding=True,
            input_names=["images"],
            output_names=output_names,
            dynamic_axes=dynamic_axes,
            dynamo=False,
        )
        if simplify:
            onnx.save(onnxslim.slim(onnx.load(str(onnx_path))), str(onnx_path))
        if dynamic:
            self.fix_batch_only_dynamic(onnx_path, size, max_det)


if __name__ == "__main__":
    run_export_cli(
        YoloSegNonE2EExporter(),
        "Export YOLO11-seg ONNX (EfficientNMSX_TRT + ROIAlignX_TRT) for NvDsInferYoloMask",
        default_max_det=100,
    )
