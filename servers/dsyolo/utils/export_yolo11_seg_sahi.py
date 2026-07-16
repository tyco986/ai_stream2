import argparse
import sys
from copy import deepcopy
from pathlib import Path

import onnx
import torch
import torch.nn as nn
import ultralytics.models.yolo
import ultralytics.utils
import ultralytics.utils.tal as _m
from ultralytics import YOLO
from ultralytics.nn.modules import C2f, Detect, RTDETRDecoder

sys.modules["ultralytics.yolo"] = ultralytics.models.yolo
sys.modules["ultralytics.yolo.utils"] = ultralytics.utils


def dist2bbox(distance, anchor_points, xywh=False, dim=-1):
    lt, rb = distance.chunk(2, dim)
    x1y1 = anchor_points - lt
    x2y2 = anchor_points + rb
    return torch.cat([x1y1, x2y2], dim)


_m.dist2bbox.__code__ = dist2bbox.__code__


class RoiAlign(torch.autograd.Function):
    @staticmethod
    def forward(
        ctx,
        X,
        rois,
        batch_indices,
        coordinate_transformation_mode,
        mode,
        output_height,
        output_width,
        sampling_ratio,
        spatial_scale,
    ):
        num_rois = rois.shape[0]
        return torch.randn(
            [num_rois, X.shape[1], output_height, output_width],
            device=rois.device,
            dtype=rois.dtype,
        )

    @staticmethod
    def symbolic(
        g,
        X,
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
            X,
            rois,
            batch_indices,
            coordinate_transformation_mode_i=coordinate_transformation_mode,
            mode_i=mode,
            output_height_i=output_height,
            output_width_i=output_width,
            sampling_ratio_i=sampling_ratio,
            spatial_scale_f=spatial_scale,
        )


class NMS(torch.autograd.Function):
    @staticmethod
    def forward(ctx, boxes, scores, score_threshold, iou_threshold, max_output_boxes):
        batch_size = scores.shape[0]
        num_classes = scores.shape[-1]
        num_detections = torch.randint(0, max_output_boxes, (batch_size, 1), dtype=torch.int32)
        detection_boxes = torch.randn(batch_size, max_output_boxes, 4)
        detection_scores = torch.randn(batch_size, max_output_boxes)
        detection_classes = torch.randint(
            0, num_classes, (batch_size, max_output_boxes), dtype=torch.int32
        )
        detections_indices = torch.randint(
            0, max_output_boxes, (batch_size, max_output_boxes), dtype=torch.int32
        )
        return num_detections, detection_boxes, detection_scores, detection_classes, detections_indices

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


class DeepStreamSahiSegOutput(nn.Module):
    def __init__(self, nc, conf_threshold, iou_threshold, max_detections):
        super().__init__()
        self.nc = nc
        self.conf_threshold = conf_threshold
        self.iou_threshold = iou_threshold
        self.max_detections = max_detections

    def forward(self, x):
        preds = x[0].transpose(1, 2)
        boxes = preds[:, :, :4]
        scores = preds[:, :, 4 : self.nc + 4]
        masks = preds[:, :, self.nc + 4 :]
        protos = x[1]

        num_detections, detection_boxes, detection_scores, detection_classes, detections_indices = (
            NMS.apply(boxes, scores, self.conf_threshold, self.iou_threshold, self.max_detections)
        )

        batch_size, num_protos, h_protos, w_protos = protos.shape
        total_detections = batch_size * self.max_detections

        batch_index = torch.ones_like(detections_indices) * torch.arange(
            batch_size, device=boxes.device, dtype=torch.int32
        ).unsqueeze(1)
        batch_index = batch_index.view(total_detections).to(torch.int32)
        box_index = detections_indices.view(total_detections).to(torch.int32)

        selected_boxes = boxes[batch_index, box_index]
        selected_masks = masks[batch_index, box_index]

        pooled_proto = RoiAlign.apply(
            protos, selected_boxes, batch_index, 1, 1, int(h_protos), int(w_protos), 0, 0.25
        )

        masks_protos = torch.matmul(
            selected_masks.unsqueeze(1),
            pooled_proto.view(total_detections, num_protos, h_protos * w_protos),
        )
        det_masks = masks_protos.sigmoid().view(batch_size, self.max_detections, h_protos * w_protos)

        result = (
            num_detections,
            detection_boxes,
            detection_scores,
            detection_classes,
            det_masks,
        )
        return result


def write_labels(model: YOLO, labels_path: Path) -> None:
    lines = [str(name) for name in model.names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def fix_batch_only_dynamic(onnx_path: Path, size: int) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")

    dims[1].ClearField("dim_param")
    dims[1].dim_value = 3
    dims[2].ClearField("dim_param")
    dims[2].dim_value = size
    dims[3].ClearField("dim_param")
    dims[3].dim_value = size

    onnx.save(model, str(onnx_path))


def yolo11_seg_export(weights: Path, device: torch.device, fuse: bool = True) -> nn.Module:
    model = YOLO(str(weights))
    model = deepcopy(model.model).to(device)
    for param in model.parameters():
        param.requires_grad = False
    model.eval()
    model.float()
    if fuse:
        model = model.fuse()
    for module in model.modules():
        if isinstance(module, (Detect, RTDETRDecoder)):
            module.dynamic = False
            module.export = True
            module.format = "onnx"
        elif isinstance(module, C2f):
            module.forward = module.forward_split
    return model


def export_yolo11_seg_sahi(
    weights: Path,
    size: int,
    opset: int,
    batch: int,
    dynamic: bool,
    simplify: bool,
    conf_threshold: float,
    iou_threshold: float,
    max_detections: int,
) -> None:
    yolo = YOLO(str(weights))
    write_labels(yolo, Path("labels.txt"))

    device = torch.device("cpu")
    core = yolo11_seg_export(weights, device)
    model = nn.Sequential(
        core,
        DeepStreamSahiSegOutput(
            len(yolo.names),
            conf_threshold,
            iou_threshold,
            max_detections,
        ),
    )

    onnx_path = weights.with_suffix(".onnx")
    onnx_input = torch.zeros(batch, 3, size, size, device=device)
    output_names = ["num_dets", "det_boxes", "det_scores", "det_classes", "det_masks"]
    dynamic_axes = None
    if dynamic:
        dynamic_axes = {
            "images": {0: "batch"},
            "num_dets": {0: "batch"},
            "det_boxes": {0: "batch"},
            "det_scores": {0: "batch"},
            "det_classes": {0: "batch"},
            "det_masks": {0: "batch"},
        }

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
        import onnxslim

        model_onnx = onnx.load(str(onnx_path))
        model_onnx = onnxslim.slim(model_onnx)
        onnx.save(model_onnx, str(onnx_path))

    if dynamic:
        fix_batch_only_dynamic(onnx_path, size)


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Export YOLO11-Seg to SAHI ONNX (images + EfficientNMS mask outputs) "
            "for deepstream-sahi NvDsInferYoloMask"
        )
    )
    parser.add_argument("-w", "--weights", required=True, help="Path to .pt weights")
    parser.add_argument("-s", "--size", type=int, default=640, help="Square input size")
    parser.add_argument("--opset", type=int, default=17, help="ONNX opset")
    parser.add_argument("--batch", type=int, default=1, help="Static batch size")
    parser.add_argument("--dynamic", action="store_true", help="Dynamic batch axis")
    parser.add_argument("--simplify", action="store_true", help="ONNX graph simplify")
    parser.add_argument("--conf-threshold", type=float, default=0.25, help="NMS confidence threshold")
    parser.add_argument("--iou-threshold", type=float, default=0.45, help="NMS IoU threshold")
    parser.add_argument(
        "--max-detections", type=int, default=100, help="Maximum detections per image"
    )
    args = parser.parse_args()

    if args.dynamic and args.batch > 1:
        raise SystemExit("Cannot set dynamic batch-size and static batch-size at same time")

    export_yolo11_seg_sahi(
        weights=Path(args.weights),
        size=args.size,
        opset=args.opset,
        batch=args.batch,
        dynamic=args.dynamic,
        simplify=args.simplify,
        conf_threshold=args.conf_threshold,
        iou_threshold=args.iou_threshold,
        max_detections=args.max_detections,
    )


if __name__ == "__main__":
    main()
