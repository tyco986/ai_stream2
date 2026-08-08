import argparse
from pathlib import Path

import onnx


def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def set_dim_value(dim, value: int) -> None:
    dim.ClearField("dim_param")
    dim.dim_value = value


def fix_batch_only_dynamic(onnx_path: Path, size: int, max_det: int) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")
    set_dim_value(dims[1], 3)
    set_dim_value(dims[2], size)
    set_dim_value(dims[3], size)

    outputs = list(model.graph.output)
    if not outputs:
        raise ValueError("onnx missing outputs")

    out0_dims = outputs[0].type.tensor_type.shape.dim
    if len(out0_dims) != 3:
        raise ValueError(f"expected output0 rank 3, got {len(out0_dims)}")
    set_dim_value(out0_dims[1], max_det)

    if len(outputs) == 1:
        set_dim_value(out0_dims[2], 6)
    else:
        out1_dims = outputs[1].type.tensor_type.shape.dim
        if len(out1_dims) != 4:
            raise ValueError(f"expected output1 rank 4, got {len(out1_dims)}")
        mask_channels = int(out1_dims[1].dim_value) if out1_dims[1].dim_value else 32
        set_dim_value(out1_dims[1], mask_channels)
        mask_size = size // 4
        set_dim_value(out1_dims[2], mask_size)
        set_dim_value(out1_dims[3], mask_size)
        if out0_dims[2].dim_param or not out0_dims[2].dim_value:
            set_dim_value(out0_dims[2], 6 + mask_channels)

    onnx.save(model, str(onnx_path))


def run_export_cli(exporter, description: str, default_max_det: int = 30) -> None:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("-w", "--weights", required=True, help="Path to .pt weights")
    parser.add_argument("-s", "--size", type=int, default=640, help="Square input size")
    parser.add_argument("--opset", type=int, default=17, help="ONNX opset")
    parser.add_argument("--batch", type=int, default=1, help="Static batch size")
    parser.add_argument("--dynamic", action="store_true", help="Dynamic batch axis")
    parser.add_argument("--simplify", action="store_true", help="ONNX graph simplify")
    parser.add_argument("--conf", type=float, default=0.25, help="Confidence threshold")
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
        max_det=args.max_det,
        conf=args.conf,
        output_dir=Path.cwd(),
    )
