import argparse
from pathlib import Path

import onnx
from ultralytics import YOLO


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


def export_yolo26_sahi(
    weights: Path,
    size: int,
    opset: int,
    batch: int,
    dynamic: bool,
    simplify: bool,
) -> None:
    model = YOLO(str(weights))
    write_labels(model, Path("labels.txt"))

    export_kwargs = {
        "format": "onnx",
        "imgsz": size,
        "opset": opset,
        "simplify": simplify,
    }
    if dynamic:
        export_kwargs["dynamic"] = True
    else:
        export_kwargs["batch"] = batch

    model.export(**export_kwargs)

    if dynamic:
        fix_batch_only_dynamic(weights.with_suffix(".onnx"), size)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Export YOLO26 to Ultralytics SAHI ONNX (images/output0) for deepstream-sahi"
    )
    parser.add_argument("-w", "--weights", required=True, help="Path to .pt weights")
    parser.add_argument("-s", "--size", type=int, default=640, help="Square input size")
    parser.add_argument("--opset", type=int, default=17, help="ONNX opset")
    parser.add_argument("--batch", type=int, default=1, help="Static batch size")
    parser.add_argument("--dynamic", action="store_true", help="Dynamic batch axis")
    parser.add_argument("--simplify", action="store_true", help="ONNX graph simplify")
    args = parser.parse_args()

    export_yolo26_sahi(
        weights=Path(args.weights),
        size=args.size,
        opset=args.opset,
        batch=args.batch,
        dynamic=args.dynamic,
        simplify=args.simplify,
    )


if __name__ == "__main__":
    main()
