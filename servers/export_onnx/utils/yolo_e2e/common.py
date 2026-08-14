from pathlib import Path

import onnx


def write_labels(names, labels_path: Path) -> None:
    lines = [str(name) for name in names.values()]
    labels_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def validate_export_args(weights: Path, dynamic: bool, batch: int) -> None:
    if dynamic and batch > 1:
        raise ValueError("dynamic batch and static batch > 1 are incompatible")
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
