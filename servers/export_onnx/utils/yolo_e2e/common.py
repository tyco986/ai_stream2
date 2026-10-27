import json
from pathlib import Path
from typing import Literal

import onnx


def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def validate_export_args(weights: Path, batch_mode: Literal["dynamic", "static"], batch_size: int) -> None:
    if not weights.is_file():
        raise ValueError(f"invalid weights file: {weights}")


def set_dim_value(dim, value: int) -> None:
    dim.ClearField("dim_param")
    dim.dim_value = value


def onnx_shape(value_info) -> list:
    dims = []
    for dim in value_info.type.tensor_type.shape.dim:
        if dim.dim_param or not dim.dim_value:
            dims.append(None)
        else:
            dims.append(int(dim.dim_value))
    return dims


def write_bundle_meta(
    output_dir: Path,
    onnx_path: Path,
    labels_path: Path,
    version: str,
    max_det: int,
    conf: float,
    iou: float,
    batch_mode: Literal["dynamic", "static"],
    batch_size: int,
) -> None:
    model = onnx.load(str(onnx_path))
    classes = [
        line.strip()
        for line in labels_path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    meta = {
        "batch_mode": batch_mode,
        "input_tensors": {
            value.name: onnx_shape(value) for value in model.graph.input
        },
        "output_tensors": {
            value.name: onnx_shape(value) for value in model.graph.output
        },
        "classes": classes,
        "num_class": len(classes),
        "source_path": str(onnx_path),
        "batch_size": None if batch_mode == "dynamic" else batch_size,
        "precision": "fp32",
        "version": version,
        "max_det": max_det,
        "conf": conf,
        "iou": iou,
    }
    (output_dir / "meta.json").write_text(
        json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


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


def pose_output_channels(num_keypoints: int) -> int:
    return 6 + 3 * num_keypoints


def fix_pose_batch_only_dynamic(
    onnx_path: Path, size: int, max_det: int, channels: int
) -> None:
    model = onnx.load(str(onnx_path))
    inp = model.graph.input[0]
    dims = inp.type.tensor_type.shape.dim
    if len(dims) != 4:
        raise ValueError(f"expected NCHW input, got {len(dims)} dims")
    set_dim_value(dims[1], 3)
    set_dim_value(dims[2], size)
    set_dim_value(dims[3], size)

    outputs = list(model.graph.output)
    if len(outputs) != 1:
        raise ValueError(f"expected 1 pose output, got {len(outputs)}")
    out0_dims = outputs[0].type.tensor_type.shape.dim
    if len(out0_dims) != 3:
        raise ValueError(f"expected output0 rank 3, got {len(out0_dims)}")
    set_dim_value(out0_dims[1], max_det)
    set_dim_value(out0_dims[2], channels)
    onnx.save(model, str(onnx_path))



def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def validate_export_args(weights: Path, batch_mode: Literal["dynamic", "static"], batch_size: int) -> None:
    if not weights.is_file():
        raise ValueError(f"invalid weights file: {weights}")


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
