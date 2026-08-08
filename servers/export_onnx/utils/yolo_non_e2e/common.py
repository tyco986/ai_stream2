import argparse
import sys
import warnings
from copy import deepcopy
from pathlib import Path

import onnx
import torch
import torch.nn as nn
import ultralytics.models.yolo
import ultralytics.utils
import ultralytics.utils.tal as tal
from ultralytics import YOLO
from ultralytics.nn.modules import C2f, Detect, RTDETRDecoder

sys.modules["ultralytics.yolo"] = ultralytics.models.yolo
sys.modules["ultralytics.yolo.utils"] = ultralytics.utils


def dist2bbox_xyxy(distance, anchor_points, xywh=False, dim=-1):
    lt, rb = distance.chunk(2, dim)
    x1y1 = anchor_points - lt
    x2y2 = anchor_points + rb
    return torch.cat([x1y1, x2y2], dim)


tal.dist2bbox.__code__ = dist2bbox_xyxy.__code__


def suppress_export_warnings() -> None:
    warnings.filterwarnings("ignore", category=torch.jit.TracerWarning)
    warnings.filterwarnings("ignore", category=UserWarning)
    warnings.filterwarnings("ignore", category=DeprecationWarning)
    warnings.filterwarnings("ignore", category=FutureWarning)
    warnings.filterwarnings("ignore", category=ResourceWarning)


def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def set_dim_value(dim, value: int) -> None:
    dim.ClearField("dim_param")
    dim.dim_value = value


def fix_batch_only_dynamic(onnx_path: Path, size: int, max_det: int | None = None) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")
    set_dim_value(dims[1], 3)
    set_dim_value(dims[2], size)
    set_dim_value(dims[3], size)

    outputs = list(model.graph.output)
    if max_det is not None and len(outputs) == 1:
        out0_dims = outputs[0].type.tensor_type.shape.dim
        if len(out0_dims) != 3:
            raise ValueError(f"expected output0 rank 3, got {len(out0_dims)}")
        set_dim_value(out0_dims[1], max_det)
        set_dim_value(out0_dims[2], 6)

    onnx.save(model, str(onnx_path))


def prepare_yolo_core(
    weights: Path, device: torch.device, max_det: int | None = None
) -> nn.Module:
    yolo = YOLO(str(weights))
    core = deepcopy(yolo.model).to(device)
    for param in core.parameters():
        param.requires_grad = False
    core.eval()
    core.float()
    core = core.fuse()
    for module in core.modules():
        if isinstance(module, (Detect, RTDETRDecoder)):
            module.dynamic = False
            module.export = True
            module.format = "onnx"
            if max_det is not None and hasattr(module, "max_det"):
                module.max_det = max_det
        elif isinstance(module, C2f):
            module.forward = module.forward_split
    return core


def run_export_cli(exporter, description: str, default_max_det: int = 30) -> None:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("-w", "--weights", required=True, help="Path to .pt weights")
    parser.add_argument("-s", "--size", type=int, default=640, help="Square input size")
    parser.add_argument("--opset", type=int, default=17, help="ONNX opset")
    parser.add_argument("--batch", type=int, default=1, help="Static batch size")
    parser.add_argument("--dynamic", action="store_true", help="Dynamic batch axis")
    parser.add_argument("--simplify", action="store_true", help="ONNX graph simplify")
    parser.add_argument("--conf", type=float, default=0.25, help="NMS confidence")
    parser.add_argument("--iou", type=float, default=0.45, help="NMS IoU")
    parser.add_argument(
        "--max-det",
        type=int,
        default=default_max_det,
        help="Maximum detections per image",
    )
    args = parser.parse_args()
    if args.dynamic and args.batch > 1:
        raise SystemExit("Cannot set dynamic batch-size and static batch-size at same time")
    if not Path(args.weights).is_file():
        raise SystemExit(f"Invalid weights file: {args.weights}")
    exporter.export(
        weights=Path(args.weights),
        size=args.size,
        opset=args.opset,
        batch=args.batch,
        dynamic=args.dynamic,
        simplify=args.simplify,
        conf=args.conf,
        iou=args.iou,
        max_det=args.max_det,
        output_dir=Path.cwd(),
    )
